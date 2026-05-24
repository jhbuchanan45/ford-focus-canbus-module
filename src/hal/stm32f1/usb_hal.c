/**
 * usb_hal.c — STM32F103 USB CDC-ACM implementation (libopencm3)
 *
 * Exposes a virtual serial port at 115200 baud over the Blue Pill USB
 * connector (PA11 D-, PA12 D+).
 *
 * Note: CAN1 is remapped to PB8/PB9 (can_hal.c) so PA11/PA12 are free
 * for USB.
 *
 * Pull-up: Many Blue Pill boards have a fixed 1k5 pull-up on D+.  If yours
 * uses a software-controlled pull-up on PA8, set USB_PULLUP_GPIO below.
 *
 * Verification:
 *   Connect Blue Pill to a Linux laptop.  dmesg should show:
 *     usb X-Y: new full-speed USB device
 *     cdc_acm X-Y:1.0: ttyACM0: USB ACM device
 *   Open /dev/ttyACM0 and confirm JSON lines appear.
 */

#include "../usb_hal.h"

#include <stdlib.h>
#include <string.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/cm3/nvic.h>

/* -------------------------------------------------------------------------
 * USB device / configuration descriptors
 * ---------------------------------------------------------------------- */

static const struct usb_device_descriptor dev_desc = {
    .bLength            = USB_DT_DEVICE_SIZE,
    .bDescriptorType    = USB_DT_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = USB_CLASS_CDC,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = 64,
    .idVendor           = 0x0483,   /* STMicroelectronics */
    .idProduct          = 0x5740,   /* Virtual COM Port   */
    .bcdDevice          = 0x0200,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor comm_endp[] = {{
    .bLength          = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x83,
    .bmAttributes     = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize   = 16,
    .bInterval        = 255,
}};

static const struct usb_endpoint_descriptor data_endp[] = {{
    .bLength          = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x01,
    .bmAttributes     = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize   = 64,
    .bInterval        = 1,
}, {
    .bLength          = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x82,
    .bmAttributes     = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize   = 64,
    .bInterval        = 1,
}};

static const struct {
    struct usb_cdc_header_descriptor header;
    struct usb_cdc_call_management_descriptor call_mgmt;
    struct usb_cdc_acm_descriptor acm;
    struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
    .header = {
        .bFunctionLength    = sizeof(struct usb_cdc_header_descriptor),
        .bDescriptorType    = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
        .bcdCDC             = 0x0110,
    },
    .call_mgmt = {
        .bFunctionLength    = sizeof(struct usb_cdc_call_management_descriptor),
        .bDescriptorType    = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
        .bmCapabilities     = 0,
        .bDataInterface     = 1,
    },
    .acm = {
        .bFunctionLength    = sizeof(struct usb_cdc_acm_descriptor),
        .bDescriptorType    = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_ACM,
        .bmCapabilities     = 0,
    },
    .cdc_union = {
        .bFunctionLength        = sizeof(struct usb_cdc_union_descriptor),
        .bDescriptorType        = CS_INTERFACE,
        .bDescriptorSubtype     = USB_CDC_TYPE_UNION,
        .bControlInterface      = 0,
        .bSubordinateInterface0 = 1,
    },
};

static const struct usb_interface_descriptor comm_iface[] = {{
    .bLength              = USB_DT_INTERFACE_SIZE,
    .bDescriptorType      = USB_DT_INTERFACE,
    .bInterfaceNumber     = 0,
    .bAlternateSetting    = 0,
    .bNumEndpoints        = 1,
    .bInterfaceClass      = USB_CLASS_CDC,
    .bInterfaceSubClass   = USB_CDC_SUBCLASS_ACM,
    .bInterfaceProtocol   = USB_CDC_PROTOCOL_AT,
    .iInterface           = 0,
    .endpoint             = comm_endp,
    .extra                = &cdcacm_functional_descriptors,
    .extralen             = sizeof(cdcacm_functional_descriptors),
}};

static const struct usb_interface_descriptor data_iface[] = {{
    .bLength            = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 1,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 2,
    .bInterfaceClass    = USB_CLASS_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
    .endpoint           = data_endp,
}};

static const struct usb_interface ifaces[] = {{
    .num_altsetting = 1,
    .altsetting     = comm_iface,
}, {
    .num_altsetting = 1,
    .altsetting     = data_iface,
}};

static const struct usb_config_descriptor config_desc = {
    .bLength             = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType     = USB_DT_CONFIGURATION,
    .wTotalLength        = 0,   /* filled by stack */
    .bNumInterfaces      = 2,
    .bConfigurationValue = 1,
    .iConfiguration      = 0,
    .bmAttributes        = 0x80, /* bus-powered */
    .bMaxPower           = 50,   /* 100 mA */
    .interface           = ifaces,
};

static const char *usb_strings[] = {
    "canmod",
    "Ford Focus MS-CAN Decoder",
    "000001",
};

/* -------------------------------------------------------------------------
 * Runtime state
 * ---------------------------------------------------------------------- */

static usbd_device *s_usbd;
static uint8_t      s_usb_ctrl_buf[128];
static volatile int s_usb_ready; /* set when SET_CONFIGURATION received */

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
    (void)wValue;

    usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

    s_usb_ready = 1;
}

static enum usbd_request_return_codes cdcacm_control_request(
        usbd_device *usbd_dev,
        struct usb_setup_data *req,
        uint8_t **buf, uint16_t *len,
        void (**complete)(usbd_device *, struct usb_setup_data *))
{
    (void)usbd_dev; (void)buf; (void)len; (void)complete;

    /* Accept SET_LINE_CODING and SET_CONTROL_LINE_STATE silently */
    if (req->bRequest == USB_CDC_REQ_SET_CONTROL_LINE_STATE ||
        req->bRequest == USB_CDC_REQ_SET_LINE_CODING) {
        return USBD_REQ_HANDLED;
    }
    return USBD_REQ_NOTSUPP;
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

void usb_hal_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);

    /* PA12 = USB_DP: drive low briefly to force device re-enumeration */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
    gpio_clear(GPIOA, GPIO12);
    for (volatile int i = 0; i < 800000; i++) {}

    s_usbd = usbd_init(&st_usbfs_v1_usb_driver, &dev_desc, &config_desc,
                       usb_strings, 3,
                       s_usb_ctrl_buf, sizeof(s_usb_ctrl_buf));

    usbd_register_set_config_callback(s_usbd, cdcacm_set_config);
    usbd_register_control_callback(s_usbd,
        USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE  | USB_REQ_TYPE_RECIPIENT,
        cdcacm_control_request);
}

void usb_hal_poll(void)
{
    usbd_poll(s_usbd);
}

void usb_hal_write(const uint8_t *buf, uint16_t len)
{
    if (!s_usb_ready) return;

    uint16_t sent = 0;
    while (sent < len) {
        uint16_t chunk = len - sent;
        if (chunk > 64) chunk = 64;
        uint16_t n = usbd_ep_write_packet(s_usbd, 0x82, buf + sent, chunk);
        if (n == 0) break; /* TX buffer full — drop remaining */
        sent += n;
    }
}
