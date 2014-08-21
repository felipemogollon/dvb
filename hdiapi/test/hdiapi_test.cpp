#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>

#define LOG_TAG "hdiapi_test"
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__);
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);

extern void ci_test_main(int argc, char** argv);
extern void demod_test_main(int argc, char** argv);
extern void demux_test_main(int argc, char** argv);
extern int av_test_main(int argc, char **argv);

int main(int argc, char** argv) {
    ALOGD("%s, entering...\n", __FUNCTION__);

    if (argc == 1) {
        ALOGD("%s, do nothing.\n", __FUNCTION__);
    } else if (argc == 2) {
        if (!strcasecmp(argv[1], "ci")) {
            ALOGD("%s, going to do ci test.\n", __FUNCTION__);
            ci_test_main(argc, argv);
        } else if (!strcasecmp(argv[1], "demod")) {
            ALOGD("%s, going to do demod test.\n", __FUNCTION__);
            demod_test_main(argc, argv);
        } else if (!strcasecmp(argv[1], "demux")) {
            ALOGD("%s, going to do demux test.\n", __FUNCTION__);
            demux_test_main(argc, argv);
        } else if (!strcasecmp(argv[1], "av")) {
            ALOGD("%s, going to do demux test.\n", __FUNCTION__);
            av_test_main(argc, argv);
        }
    }

    ALOGD("%s, exiting...\n", __FUNCTION__);

    return 0;
}

