/**
 * test_regression.c — Golden-file regression test
 *
 * Replays tools/captures/forscan_baseline.log through canmod-host and
 * compares the JSON output (with "ts" fields stripped) against
 * tools/captures/forscan_baseline_golden.ndjson.
 *
 * Skipped (exits 0) if either capture file is absent — CI passes from
 * day one without the files, and the test activates automatically once
 * they are committed.
 *
 * CANMOD_HOST_PATH is defined by CMakeLists.txt at build time.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef CANMOD_HOST_PATH
#define CANMOD_HOST_PATH "./canmod-host"
#endif

#define LOG_PATH    "tools/captures/forscan_baseline.log"
#define GOLDEN_PATH "tools/captures/forscan_baseline_golden.ndjson"
#define TMP_ACTUAL  "/tmp/canmod_regression_actual.ndjson"
#define TMP_GOLDEN  "/tmp/canmod_regression_golden_stripped.ndjson"

static int file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

int main(void)
{
    /* Check if capture files exist */
    if (!file_exists(LOG_PATH) || !file_exists(GOLDEN_PATH)) {
        printf("SKIP: capture files not present (%s or %s missing)\n"
               "      Run task 7.2 (FORScan capture) to activate this test.\n",
               LOG_PATH, GOLDEN_PATH);
        return 0;
    }

    printf("Running regression test against %s\n", GOLDEN_PATH);

    /* Step 1: replay the log through canmod-host, capture JSON output.
     * Strip "ts":<number> fields before comparing. */
    char cmd[512];

    /* Create a vcan0 if not already present (best-effort, may need sudo) */
    system("sudo ip link add dev vcan0 type vcan 2>/dev/null || true");
    system("sudo ip link set up vcan0 2>/dev/null || true");

    /* Launch canmod-host in background, redirect stdout to tmp file */
    snprintf(cmd, sizeof(cmd),
        "%s --interface vcan0 > %s 2>/dev/null &", CANMOD_HOST_PATH, TMP_ACTUAL);
    system(cmd);

    /* Give it a moment to open the socket */
    system("sleep 0.2");

    /* Replay the log */
    snprintf(cmd, sizeof(cmd), "canplayer -I %s -l 1 vcan0=can0 2>/dev/null || true", LOG_PATH);
    system(cmd);

    /* Wait for output to flush */
    system("sleep 0.3");

    /* Kill canmod-host */
    system("pkill -f canmod-host 2>/dev/null || true");

    /* Step 2: strip "ts":<number>, fields from both files for comparison */
    snprintf(cmd, sizeof(cmd),
        "sed 's/\"ts\":[0-9]*,//g' %s > %s.stripped 2>/dev/null", TMP_ACTUAL, TMP_ACTUAL);
    system(cmd);

    snprintf(cmd, sizeof(cmd),
        "sed 's/\"ts\":[0-9]*,//g' %s > %s 2>/dev/null", GOLDEN_PATH, TMP_GOLDEN);
    system(cmd);

    /* Step 3: diff */
    snprintf(cmd, sizeof(cmd), "diff -u %s %s.stripped", TMP_GOLDEN, TMP_ACTUAL);
    int rc = system(cmd);

    if (rc == 0) {
        printf("PASS: regression output matches golden file\n");
        return 0;
    } else {
        printf("FAIL: regression output differs from golden file\n");
        printf("      Golden: %s\n", GOLDEN_PATH);
        printf("      Actual: %s.stripped\n", TMP_ACTUAL);
        printf("      To update golden: cp %s.stripped %s\n", TMP_ACTUAL, GOLDEN_PATH);
        return 1;
    }
}
