#include "Mbed.hpp"
// #include "Rtos.hpp"
#include "LocalFileSystem.h"

#include <errno.h>
#include <string>
#include <vector>

LocalFileSystem local("local");

int fileCopy(const char* local_file1, const char* local_file2) {
    FILE *rp = fopen(local_file1, "rb");
    if (rp == NULL) {
        return -1;
    }
    remove(local_file2);
    FILE *wp = fopen(local_file2, "wb");
    if (wp == NULL) {
        fclose(rp);
        return -2;
    }
    // int c;
    // while ((c = fgetc(rp)) != EOF) {
    //     fputc(c, wp);
    // }
    uint8_t temp[1024];
    while (!feof(rp)) {
        // printf("b");
        int count = fread(temp, 1, 1024, rp);
        fwrite(temp, 1, count, wp);
        // Thread::wait(10);
    }
    fclose(rp);
    fclose(wp);
    return 0;
}

int main() {
    Serial s(MBED_UARTUSB);
    s.baud(57600);


    printf("****************************************\r\n");

    // FILE *fp = fopen("/local/rj-fpga.nib", "rb");

    // fseek(fp, 0, SEEK_END);
    // long lSize = ftell(fp);
    // rewind(fp);

    // uint8_t* buffer = (uint8_t*) malloc(sizeof(uint8_t)*50);
    // fread (buffer, 1, 50, fp);

    // fopen("/local/test.txt", "r");

    // int ret = rename("/local/test.txt", "/local/test2.txt");
    // printf("Error: %s\r\n", strerror(errno));
    // printf("ret = %d\r\n", ret);

    // set_time(1000000);
    // auto fp = fopen("/local/t2.txt", "w");
    // fclose(fp);

    printf("Start copy\r\n");
    remove("/local/temp.tmp");
    fileCopy("/local/rj-ctrl.bin", "/local/temp.tmp");
    printf("End copy\r\n");

    DIR *d = opendir("/local/");
    std::vector<std::string> filenames;
    struct dirent* p;
    while ((p = readdir(d)) != nullptr) {
        filenames.push_back(string(p->d_name));
    }

    closedir(d);

    for (auto& name : filenames) {
        printf(" - %s\r\n", name.c_str());
    }

}
