#include <stdio.h>
#include <string.h>
#include <android/log.h>

#include "version.h"

#define LOG_TAG "hdi_version"
#define log_print(...) __android_log_print(ANDROID_LOG_SILENT, LOG_TAG, __VA_ARGS__)

static char versioninfo[256] = "N/A";
static long long version_serial = 0;
static char gitversionstr[256] = "N/A";

static int hdi_version_info_init(void) {
    static int info_is_inited = 0;
    char git_shor_version[20];
    unsigned int shortgitversion;
    int dirty_num = 0;

    if (info_is_inited > 0) {
        return 0;
    }
    info_is_inited++;

#ifdef HAVE_VERSION_INFO
#ifdef LIBHDI_GIT_UNCOMMIT_FILE_NUM
#if LIBHDI_GIT_UNCOMMIT_FILE_NUM>0
    dirty_num = LIBHDI_GIT_UNCOMMIT_FILE_NUM;
#endif
#endif
#ifdef LIBHDI_GIT_VERSION
    if (dirty_num > 0) {
        snprintf(gitversionstr, 250, "%s-with-%d-dirty-files", LIBHDI_GIT_VERSION, dirty_num);
    } else {
        snprintf(gitversionstr, 250, "%s", LIBHDI_GIT_VERSION);
    }
#endif
#endif
    memcpy(git_shor_version, gitversionstr, 8);
    git_shor_version[8] = '\0';
    sscanf(git_shor_version, "%x", &shortgitversion);
    version_serial = (long long) (LIBHDI_VERSION_MAIN & 0xff) << 56 | (long long) (LIBHDI_VERSION_SUB1 & 0xff) << 48 | (long long) (LIBHDI_VERSION_SUB2 & 0xff) << 32
            | shortgitversion;

    snprintf(versioninfo, 256, "Version:%d.%d.%d.%s", LIBHDI_VERSION_MAIN, LIBHDI_VERSION_SUB1, LIBHDI_VERSION_SUB2, git_shor_version);

    return 0;
}

static const char *hdi_get_version_info(void) {
    hdi_version_info_init();
    return versioninfo;
}

static int64_t hdi_get_version_serail(void) {
    hdi_version_info_init();
    return version_serial;
}

static const char *hdi_get_git_version_info(void) {
    hdi_version_info_init();
    return gitversionstr;
}

static const char *hdi_get_last_chaned_time_info(void) {
#ifdef HAVE_VERSION_INFO
#ifdef LIBHDI_LAST_CHANGED
    return LIBHDI_LAST_CHANGED;
#endif
#endif
    return " Unknow ";
}

static const char *hdi_get_git_branch_info(void) {
#ifdef HAVE_VERSION_INFO
#ifdef LIBHDI_GIT_BRANCH
    return LIBHDI_GIT_BRANCH;
#endif
#endif
    return " Unknow ";
}

static const char *hdi_get_build_time_info(void) {
#ifdef HAVE_VERSION_INFO
#ifdef LIBHDI_BUILD_TIME
    return LIBHDI_BUILD_TIME;
#endif
#endif
    return " Unknow ";
}

static const char *hdi_get_build_name_info(void) {
#ifdef HAVE_VERSION_INFO
#ifdef LIBHDI_BUILD_NAME
    return LIBHDI_BUILD_NAME;
#endif
#endif
    return " Unknow ";
}

void print_version_info() {
#ifdef HAVE_VERSION_INFO
    hdi_version_info_init();

    log_print("libhdi version:%s\n", hdi_get_version_info());
    log_print("libhdi git version:%s\n", hdi_get_git_version_info());
    log_print("libhdi version serial:%llx\n", hdi_get_version_serail());
    log_print("libhdi git branch:%s\n", hdi_get_git_branch_info());
    log_print("libhdi Last Changed:%s\n", hdi_get_last_chaned_time_info());
    log_print("libhdi Last Build:%s\n", hdi_get_build_time_info());
    log_print("libhdi Builer Name:%s\n", hdi_get_build_name_info());
#endif
}
