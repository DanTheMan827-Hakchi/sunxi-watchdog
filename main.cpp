#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <linux/input.h>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>

int powerFd = -1, resetFd = -1, joyFd = -1;
const auto waitTime = std::chrono::milliseconds(200);

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y) {
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

void key_watcher(int inputFd, short keyCode, std::string type, std::string description) {
    if (inputFd == -1) {
        fprintf(stderr, "Cannot access %s state\n", description.c_str());
        return;
    }

    timeval pushTime, currentTime;
    gettimeofday(&pushTime, NULL);

    input_event event;
    int num;
    while (read(inputFd, &event, (sizeof event)) > 0) {
        if (event.type == EV_KEY && event.code == keyCode) {
            timeval diff;

            if (event.value == 1) {
                gettimeofday(&pushTime, NULL);
                fprintf(stdout, "%s %s %i\n", type.c_str(), description.c_str(), event.value);
            } else {
                gettimeofday(&currentTime, NULL);
                timeval_subtract(&diff, &currentTime, &pushTime);
                fprintf(stdout, "%s %s %i %li\n", type.c_str(), description.c_str(), event.value, (diff.tv_sec * 1000) + (diff.tv_usec / 1000));
            }
            fflush(stdout);
        }
    }
}

void pid_watcher(unsigned int pid) {
    while(kill(pid, 0) == 0)
    {
        std::this_thread::sleep_for(waitTime);
    }
    exit(0);
}

int main(int argc, char * argv[]) {
    unsigned int pid = 0;

    int i = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i],"--help") == 0) {
            fprintf(stdout, "--help       Displays this help information.\n");
            fprintf(stdout, "--pid [pid]  Exit when the specified pid terminates.\n");
            exit(0);
        }
        if (strcmp(argv[i],"--pid") == 0) {
            pid = std::stoi(argv[i + 1]);
        }
    }

    if (DIR *dir = opendir("/dev/input/")) {
        while (dirent *entry = readdir(dir)) {
            std::string name(entry->d_name);
            if (name.find("event") != std::string::npos) {
                std::ifstream in(("/sys/class/input/"+name+"/device/name").c_str());
                if (in.good()) {
                    std::string temp;
                    std::getline(in, temp);
                    in.close();
                    if(temp == "sunxi-knob") {
                        powerFd = open(("/dev/input/"+name).c_str(), O_RDONLY);
                    } else if(temp == "sunxi-keyboard") {
                        resetFd = open(("/dev/input/"+name).c_str(), O_RDONLY);
                    }
                }
            }
        }
        closedir(dir);
    } else {
        powerFd = open("power", O_RDONLY);
        resetFd = open("reset", O_RDONLY);
        fprintf(stderr, "Cannot open /dev/input/\n");
        fflush(stderr);
    }

    joyFd = open("/dev/input/by-path/platform-twi.1-event-joystick", O_RDONLY);

    std::thread joyThread(key_watcher, joyFd, BTN_MODE, "joy", "home");
    std::thread resetThread(key_watcher, resetFd, KEY_VOLUMEUP, "key", "reset");
    std::thread powerThread(key_watcher, powerFd, KEY_POWER, "key", "power");

    if (pid != 0) {
        std::thread pidThread(pid_watcher, pid);
        pidThread.join();
    }

    joyThread.join();
    resetThread.join();
    powerThread.join();
}
