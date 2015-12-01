#include "commands.hpp"

#include <map>

#include <rtos.h>
#include <mbed_rpc.h>
#include <CommModule.hpp>
#include <numparser.hpp>
#include <logger.hpp>

#include "ds2411.hpp"
#include "neostrip.hpp"

using std::string;
using std::vector;

extern struct OS_XCB os_rdy;

namespace {
/**
 * error message when a typed command isn't found
 */
const string COMMAND_NOT_FOUND_MSG =
    "Command '%s' not found. Type 'help' for a list of commands.";

/**
 * error message when too many args are provided
 */
const string TOO_MANY_ARGS_MSG = "*** too many arguments ***";

/**
 * indicates if the command held in "iterativeCommand"
 */
volatile bool iterative_command_state = false;

/**
 * current iterative command args
 */
vector<string> iterative_command_args;

/**
 * the current iterative command handler
 */
int (*iterative_command_handler)(cmd_args_t& args);

}  // end of anonymous namespace

// Create an object to help find files
LocalFileSystem local("local");

/**
 * Commands list. Add command handlers to commands.hpp.
 *
 * Alphabetical order please (here addition and in handler function
 * declaration).
 */
static const vector<command_t> commands = {
    /* command definition example
    {
        {"<alias>", "<alias2>", "<alias...>"},
        is the command iterative,
        command handler function,
        "description",
        "usage"},
    */
    {{"alias", "a"},
     false,
     cmd_alias,
     "Lists aliases for commands.",
     "alias | a"},
    {{"baud", "baudrate"},
     false,
     cmd_baudrate,
     "Set the serial link's baudrate.",
     "baud | baudrate [[--list | -l] | [<target_rate>]]"},
    {{"clear", "cls"},
     false,
     cmd_console_clear,
     "Clears the screen.",
     "clear | cls"},
    {{"echo"},
     false,
     cmd_console_echo,
     "Echos text for debugging the serial link.",
     "echo <text>"},
    {{"exit", "quit"},
     false,
     cmd_console_exit,
     "Breaks the main loop.",
     "exit | quit"},
    {{"help", "h", "?"},
     false,
     cmd_help,
     "Prints this message.",
     "help | h | ? [[--list|-l] | [--all|-a] | <command names>]"},
    {{"host", "hostname"},
     false,
     cmd_console_hostname,
     "Set the system hostname.",
     "host | hostname <new_hostname>"},
    {{"info", "version", "i"},
     false,
     cmd_info,
     "Display information about the current version of the firmware.",
     "info | version | i"},
    {{"isconn", "checkconn"},
     false,
     cmd_interface_check_conn,
     "Checks the connection with a debugging unit.",
     "isconn | checkconn"},
    {{"led"},
     false,
     cmd_led,
     "Change the color and brightness of LED(s).",
     "led [brightness|state|color] [<level>|<on|off>|<color]"},
    {{"loglvl", "loglevel"},
     false,
     cmd_log_level,
     "Change the active logging output level.",
     "loglvl | loglevel {+,-}..."},
    {{"ls", "l"},
     false,
     cmd_ls,
     "List contents of current directory",
     "ls | l [folder/device]"},
    {{"motor"},
     false,
     cmd_motors,
     "Show information about the motors.",
     "motor <motor_id>"},
    {{"motorscroll"},
     true,
     cmd_motors_scroll,
     "Continuously update the console with new motor values.",
     "motorscroll"},
    {{"ping"}, true, cmd_ping, "Check console responsiveness.", "ping"},
    {{"ps"}, false, cmd_ps, "List information about all active threads.", "ps"},
    {{"radio"},
     false,
     cmd_radio,
     "Show information about the radio & perform basic radio tasks.",
     "radio [port | [test-tx | test-rx] <port-num>] [[open, close, show, "
     "reset] "
     "<port_num>]"},
    {{"reboot", "reset", "restart"},
     false,
     cmd_interface_reset,
     "Resets the mbed (like pushing the reset button).",
     "reboot | reset | restart"},
    {{"rmdev", "unconnect"},
     false,
     cmd_interface_disconnect,
     "Disconnects the mbed interface chip from the microcontroller.",
     "rmdev | unconnect [-P]"},
    {{"rpc"},
     false,
     cmd_rpc,
     "Execute RPC commands on the mbed.",
     "rpc <rpc-command>"},
    {{"su", "user"},
     false,
     cmd_console_user,
     "Set active user.",
     "su | user <new_username>"}};

/**
* Lists aliases for commands, if args are present, it will only list aliases
* for those commands.
*/
int cmd_alias(cmd_args_t& args) {
    // If no args given, list all aliases
    if (args.empty() == true) {
        for (uint8_t i = 0; i < commands.size(); i++) {
            printf("\t%s:\t", commands[i].aliases[0].c_str());

            // print aliases
            uint8_t a = 0;

            while (a < commands[i].aliases.size() &&
                   commands[i].aliases[a] != "\0") {
                printf("%s", commands[i].aliases[a].c_str());

                // print commas
                if (a < commands[i].aliases.size() - 1 &&
                    commands[i].aliases[a + 1] != "\0") {
                    printf(", ");
                }

                a++;
            }

            printf("\r\n");
        }
    } else {
        bool aliasFound = false;

        for (uint8_t argInd = 0; argInd < args.size(); argInd++) {
            for (uint8_t cmdInd = 0; cmdInd < commands.size(); cmdInd++) {
                // check match against args
                if (find(commands[cmdInd].aliases.begin(),
                         commands[cmdInd].aliases.end(),
                         args[argInd]) != commands[cmdInd].aliases.end()) {
                    aliasFound = true;

                    printf("%s:\r\n", commands[cmdInd].aliases[0].c_str());

                    // print aliases
                    uint8_t a = 0;

                    while (a < commands[cmdInd].aliases.size() &&
                           commands[cmdInd].aliases[a] != "\0") {
                        printf("\t%s", commands[cmdInd].aliases[a].c_str());

                        // print commas
                        if (a < commands[cmdInd].aliases.size() - 1 &&
                            commands[cmdInd].aliases[a + 1] != "\0") {
                            printf(",");
                        }

                        a++;
                    }
                }
            }

            if (aliasFound == false) {
                printf("Error listing aliases: command '%s' not found",
                       args[argInd].c_str());
            }

            printf("\r\n");
        }
    }

    return 0;
}

/**
* Clears the console.
*/
int cmd_console_clear(cmd_args_t& args) {
    if (args.empty() == false) {
        show_invalid_args(args);
        return 1;
    } else {
        Console::Flush();
        printf(ENABLE_SCROLL_SEQ.c_str());
        printf(CLEAR_SCREEN_SEQ.c_str());
    }

    return 0;
}

/**
 * Echos text.
 */
int cmd_console_echo(cmd_args_t& args) {
    for (uint8_t argInd = 0; argInd < args.size(); argInd++)
        printf("%s ", args[argInd].c_str());

    printf("\r\n");

    return 0;
}

/**
 * Requests a system stop. (breaks main loop, or whatever implementation this
 * links to).
 */
int cmd_console_exit(cmd_args_t& args) {
    if (args.empty() == false) {
        show_invalid_args(args);
        return 1;
    } else {
        printf("Shutting down serial console. Goodbye!\r\n");
        Console::RequestSystemStop();
    }

    return 0;
}

/**
 * Prints command help.
 */
int cmd_help(cmd_args_t& args) {
    // printf("\r\nCtrl + C stops iterative commands\r\n\r\n");

    // Prints all commands, with details
    if (args.empty() == true) {
        // Default to a short listing of all the commands
        for (size_t i = 0; i < commands.size(); i++)
            printf("\t%s:\t%s\r\n", commands[i].aliases[0].c_str(),
                   commands[i].description.c_str());

    }
    // Check if there's only 1 argument passed. It may be an option flag to the
    // command
    // Prints all commands - either as a list block or all detailed
    else {
        if (strcmp(args[0].c_str(), "--list") == 0 ||
            strcmp(args[0].c_str(), "-l") == 0) {
            for (uint8_t i = 0; i < commands.size(); i++) {
                if (i % 5 == 4) {
                    printf("%s\r\n", commands[i].aliases[0].c_str());
                } else if (i == commands.size() - 1) {
                    printf("%s", commands[i].aliases[0].c_str());
                } else {
                    printf("%s,\t\t", commands[i].aliases[0].c_str());
                }
            }
        } else if (strcmp(args[0].c_str(), "--all") == 0 ||
                   strcmp(args[0].c_str(), "-a") == 0) {
            for (uint8_t i = 0; i < commands.size(); i++) {
                // print info about ALL commands
                printf(
                    "%s%s:\r\n"
                    "    Description:\t%s\r\n"
                    "    Usage:\t\t%s\r\n",
                    commands[i].aliases.front().c_str(),
                    (commands[i].is_iterative ? " [ITERATIVE]" : ""),
                    commands[i].description.c_str(), commands[i].usage.c_str());
            }
        } else {
            // Show detailed info of the command given since it was not an
            // option flag
            cmd_help_detail(args);
        }

        printf("\r\n");
    }

    return 0;
}

int cmd_help_detail(cmd_args_t& args) {
    // iterate through args
    for (uint8_t argInd = 0; argInd < args.size(); argInd++) {
        // iterate through commands
        bool commandFound = false;

        for (uint8_t i = 0; i < commands.size(); i++) {
            // check match against args
            if (find(commands[i].aliases.begin(), commands[i].aliases.end(),
                     args[argInd]) != commands[i].aliases.end()) {
                commandFound = true;

                // print info about a command
                printf(
                    "%s%s:\r\n"
                    "    Description:\t%s\r\n"
                    "    Usage:\t\t%s\r\n",
                    commands[i].aliases.front().c_str(),
                    (commands[i].is_iterative ? " [ITERATIVE]" : ""),
                    commands[i].description.c_str(), commands[i].usage.c_str());
            }
        }
        // if the command wasn't found, notify
        if (!commandFound) {
            printf("Command \"%s\" not found.\r\n", args.at(argInd).c_str());
        }
    }

    return 0;
}

/**
 * Console responsiveness test.
 */
int cmd_ping(cmd_args_t& args) {
    if (args.empty() == false) {
        show_invalid_args(args);
        return 1;
    } else {
        time_t sys_time = time(nullptr);
        Console::Flush();
        printf("reply: %lu\r\n", sys_time);
        Console::Flush();

        Thread::wait(600);
    }

    return 0;
}

/**
 * Resets the mbed (should be the equivalent of pressing the reset button).
 */
int cmd_interface_reset(cmd_args_t& args) {
    if (args.empty() == false) {
        show_invalid_args(args);
        return 1;
    } else {
        printf("The system is going down for reboot NOW!\033[0J\r\n");
        Console::Flush();

        // give some time for the feedback to get back to the console
        Thread::wait(800);

        mbed_interface_reset();
    }

    return 0;
}

/**
 * Lists files.
 */
int cmd_ls(cmd_args_t& args) {
    DIR* d;
    struct dirent* p;

    std::vector<std::string> filenames;

    if (args.empty() == true) {
        d = opendir("/local");
    } else {
        d = opendir(args.front().c_str());
    }

    if (d != nullptr) {
        while ((p = readdir(d)) != nullptr) {
            filenames.push_back(string(p->d_name));
        }

        closedir(d);

        // don't use printf until we close the directory
        for (auto& i : filenames) {
            printf(" - %s\r\n", i.c_str());
            Console::Flush();
        }

    } else {
        if (args.empty() == false) {
            printf("Could not find %s\r\n", args.front().c_str());
        }

        return 1;
    }

    return 0;
}

/**
 * Prints system info.
 */
int cmd_info(cmd_args_t& args) {
    if (args.empty() == false) {
        show_invalid_args(args);
        return 1;
    } else {
        char buf[33];
        DS2411_t id;
        unsigned int Interface[5] = {0};
        time_t sys_time = time(nullptr);
        typedef void (*CallMe)(unsigned int[], unsigned int[]);

        strftime(buf, 25, "%c", localtime(&sys_time));
        printf("\tSys Time:\t%s\r\n", buf);

        // kernel information
        printf("\tKernel Ver:\t%s\r\n", osKernelSystemId);
        printf("\tAPI Ver:\t%u\r\n", osCMSIS);

        printf(
            "\tCommit Hash:\t%s%s\r\n\tCommit Date:\t%s\r\n\tCommit "
            "Author:\t%s\r\n",
            git_version_hash, git_version_dirty ? " (dirty)" : "",
            git_head_date, git_head_author);

        printf("\tBuild Date:\t%s %s\r\n", __DATE__, __TIME__);

        printf("\tBase ID:\t");

        if (ds2411_read_id(RJ_BASE_ID, &id, true) == ID_HANDSHAKE_FAIL)
            printf("N/A\r\n");
        else
            for (int i = 0; i < 6; i++) printf("%02X\r\n", id.serial[i]);

        // info about the mbed's interface chip on the bottom of the mbed
        if (mbed_interface_uid(buf) == -1) memcpy(buf, "N/A\0", 4);

        printf("\tmbed UID:\t0x%s\r\n", buf);

        // Prints out a serial number, taken from the mbed forms
        // https://developer.mbed.org/forum/helloworld/topic/2048/
        Interface[0] = 58;
        CallMe CallMe_entry = (CallMe)0x1FFF1FF1;
        CallMe_entry(Interface, Interface);

        if (!Interface[0])
            printf("\tMCU UID:\t%u %u %u %u\r\n", Interface[1], Interface[2],
                   Interface[3], Interface[4]);
        else
            printf("\tMCU UID:\t\tN/A\r\n");

        // Should be 0x26013F37
        Interface[0] = 54;
        CallMe_entry(Interface, Interface);

        if (!Interface[0])
            printf("\tMCU ID:\t\t%u\r\n", Interface[1]);
        else
            printf("\tMCU ID:\t\tN/A\r\n");

        // show info about the core processor. ARM cortex-m3 in our case
        printf("\tCPUID:\t\t0x%08lX\r\n", SCB->CPUID);

        // ** NOTE: THE mbed_interface_mac() function does not work! It hangs
        // the mbed... **

        /*
            *****
            THIS CODE IS SUPPOSED TO GIVE USB STATUS INFORMATION. IT HANGS AT
           EACH WHILE LOOP.
            [DON'T UNCOMMENT IT UNLESS YOU INTEND TO CHANGE/IMPROVE/FIX IT
           SOMEHOW]
            *****

            LPC_USB->USBDevIntClr = (0x01 << 3);    // clear the DEV_STAT
           interrupt bit before beginning
            LPC_USB->USBDevIntClr = (0x03 << 4);    // make sure CCEmpty &
           CDFull are cleared before starting
            // Sending a COMMAND transfer type for getting the [USB] device
           status. We expect 1 byte.
            LPC_USB->USBCmdCode = (0x05 << 8) | (0xFE << 16);
            while (!(LPC_USB->USBDevIntSt & 0x10)); // wait for the command
           to be completed
            LPC_USB->USBDevIntClr = 0x10;   // clear the CCEmpty interrupt bit

            // Now we request a read transfer type for getting the same thing
            LPC_USB->USBCmdCode = (0x02 << 8) | (0xFE << 16);
            while (!(LPC_USB->USBDevIntSt & 0x20)); // Wait for CDFULL. data
           ready after this in USBCmdData
            uint8_t regVal = LPC_USB->USBCmdData;   // get the byte
            LPC_USB->USBDevIntClr = 0x20;   // clear the CDFULL interrupt bit

            printf("\tUSB Byte:\t0x%02\r\n", regVal);
        */
    }

    return 0;
}

/**
 * [cmd_disconnectMbed description]
 * @param args [description]
 */
int cmd_interface_disconnect(cmd_args_t& args) {
    if (args.empty() > 1) {
        show_invalid_args(args);
        return 1;
    } else {
        if (args.empty() == true) {
            mbed_interface_disconnect();
            printf("Disconnected mbed interface.\r\n");

        } else if (strcmp(args.at(0).c_str(), "-P") == 0) {
            printf("Powering down mbed interface.\r\n");
            mbed_interface_powerdown();
        }
    }

    return 0;
}

int cmd_interface_check_conn(cmd_args_t& args) {
    if (args.empty() == false) {
        show_invalid_args(args);
        return 1;
    } else {
        printf("mbed interface connected: %s\r\n",
               mbed_interface_connected() ? "YES" : "NO");
    }

    return 0;
}

int cmd_baudrate(cmd_args_t& args) {
    std::vector<int> valid_rates = {110,   300,    600,    1200,   2400,
                                    4800,  9600,   14400,  19200,  38400,
                                    57600, 115200, 230400, 460800, 921600};

    if (args.size() > 1) {
        show_invalid_args(args);
        return 1;
    } else if (args.empty() == true) {
        printf("Baudrate: %u\r\n", Console::Baudrate());
    } else if (args.size() == 1) {
        std::string str_baud = args.front();

        if (strcmp(str_baud.c_str(), "--list") == 0 ||
            strcmp(str_baud.c_str(), "-l") == 0) {
            printf("Valid baudrates:\r\n");

            for (unsigned int i = 0; i < valid_rates.size(); i++)
                printf("%u\r\n", valid_rates[i]);

        } else if (isInt(str_baud)) {
            int new_rate = atoi(str_baud.c_str());

            if (std::find(valid_rates.begin(), valid_rates.end(), new_rate) !=
                valid_rates.end()) {
                Console::Baudrate(new_rate);
                printf("New baudrate: %u\r\n", new_rate);
            } else {
                printf(
                    "%u is not a valid baudrate. Use \"--list\" to show valid "
                    "baudrates.\r\n",
                    new_rate);
            }
        } else {
            printf("Invalid argument \"%s\".\r\n", str_baud.c_str());
        }
    }

    return 0;
}

int cmd_console_user(cmd_args_t& args) {
    if (args.empty() == true || args.size() > 1) {
        show_invalid_args(args);
        return 1;
    } else {
        Console::changeUser(args.front());
    }

    return 0;
}

int cmd_console_hostname(cmd_args_t& args) {
    if (args.empty() == true || args.size() > 1) {
        show_invalid_args(args);
        return 1;
    } else {
        Console::changeHostname(args.front());
    }

    return 0;
}

int cmd_log_level(cmd_args_t& args) {
    if (args.size() > 1) {
        show_invalid_args(args);
        return 1;
    } else if (args.empty() == true) {
        printf("Log level: %s\r\n", LOG_LEVEL_STRING[rjLogLevel]);
    } else {
        // bool storeVals = true;

        if (strcmp(args.front().c_str(), "on") == 0 ||
            strcmp(args.front().c_str(), "enable") == 0) {
            isLogging = true;
            printf("Logging enabled.\r\n");
        } else if (strcmp(args.front().c_str(), "off") == 0 ||
                   strcmp(args.front().c_str(), "disable") == 0) {
            isLogging = false;
            printf("Logging disabled.\r\n");
        } else {
            // this will return a signed int, so the level
            // could increase or decrease...or stay the same.
            int newLvl = (int)rjLogLevel;  // rjLogLevel is unsigned, so we'll
            // need to change that first
            newLvl += logLvlChange(args.front());

            if (newLvl >= LOG_LEVEL_END) {
                printf("Unable to set log level above maximum value.\r\n");
                newLvl = rjLogLevel;
            } else if (newLvl <= LOG_LEVEL_START) {
                printf("Unable to set log level below minimum value.\r\n");
                newLvl = rjLogLevel;
            }

            if (newLvl != rjLogLevel) {
                rjLogLevel = newLvl;
                printf("New log level: %s\r\n", LOG_LEVEL_STRING[rjLogLevel]);
            } else {
                // storeVals = false;
                printf("Log level unchanged. Level: %s\r\n",
                       LOG_LEVEL_STRING[rjLogLevel]);
            }
        }
    }

    return 0;
}

int cmd_rpc(cmd_args_t& args) {
    if (args.empty() == true) {
        show_invalid_args(args);
        return 1;
    } else {
        // remake the original string so it can be passed to RPC
        std::string in_buf(args.at(0));
        for (unsigned int i = 1; i < args.size(); i++) {
            in_buf += " " + args.at(i);
        }

        char out_buf[200] = {0};

        RPC::call(in_buf.c_str(), out_buf);

        std::printf("%s\r\n", out_buf);
    }

    return 0;
}

int cmd_led(cmd_args_t& args) {
    if (args.empty() == true) {
        show_invalid_args(args);
        return 1;
    } else {
        NeoStrip led(RJ_NEOPIXEL);
        led.setFromDefaultBrightness();
        led.setFromDefaultColor();

        if (strcmp(args.front().c_str(), "brightness") == 0) {
            if (args.size() > 1) {
                float bri = atof(args.at(1).c_str());
                printf("Setting LED brightness to %.2f.\r\n", bri);
                if (bri > 0 && bri <= 1.0) {
                    NeoStrip::defaultBrightness(bri);
                    led.setFromDefaultBrightness();
                    led.setFromDefaultColor();
                } else {
                    printf(
                        "Invalid brightness level of %.2f. Use 'state' command "
                        "to toggle LED's state.\r\n",
                        bri);
                }
            } else {
                printf("Current brightness:\t%.2f\r\n", led.brightness());
            }
        } else if (strcmp(args.front().c_str(), "color") == 0) {
            if (args.size() > 1) {
                std::map<std::string, NeoColor> colors;
                // order for struct is green, red, blue
                colors["red"] = {0x00, 0xFF, 0x00};
                colors["green"] = {0xFF, 0x00, 0x00};
                colors["blue"] = {0x00, 0x00, 0xFF};
                colors["yellow"] = {0xFF, 0xFF, 0x00};
                colors["purple"] = {0x00, 0xFF, 0xFF};
                colors["white"] = {0xFF, 0xFF, 0xFF};
                auto it = colors.find(args.at(1));
                if (it != colors.end()) {
                    printf("Changing color to %s.\r\n", it->first.c_str());
                    led.setPixel(1, it->second.red, it->second.green,
                                 it->second.blue);
                } else {
                    show_invalid_args(args);
                    return 1;
                }
            } else {
                return 1;
            }
            // push out the changes to the led
            led.write();
        } else if (strcmp(args.front().c_str(), "state") == 0) {
            if (args.size() > 1) {
                if (strcmp(args.at(1).c_str(), "on") == 0) {
                    printf("Turning LED on.\r\n");
                } else if (strcmp(args.at(1).c_str(), "off") == 0) {
                    printf("Turning LED off.\r\n");
                    led.brightness(0.0);
                } else {
                    show_invalid_args(args.at(1));
                    return 1;
                }
                led.setFromDefaultColor();
            } else {
                return 1;
            }
            // push out the changes to the led
            led.write();
        } else {
            show_invalid_args(args);
            return 1;
        }
    }
    return 0;
}

int cmd_ps(cmd_args_t& args) {
    if (args.empty() != true) {
        show_invalid_args(args);
        return 1;
    } else {
        unsigned int num_threads = 0;
        P_TCB p_b = (P_TCB)&os_rdy;
        std::printf("ID\tPRIOR\tB_PRIOR\tSTATE\tDELTA TIME\r\n");
        // iterate over the linked list of tasks
        while (p_b != NULL) {
            std::printf("%u\t%u\t%u\t%u\t%u\r\n", p_b->task_id, p_b->prio,
                        p_b->state, p_b->delta_time);

            num_threads++;
            p_b = p_b->p_lnk;
        }
        std::printf("==============\r\nTotal Threads:\t%u\r\n", num_threads);
    }

    return 0;
}

/**
 * [cmd_radio description]
 * @param args [description]
 */
int cmd_radio(cmd_args_t& args) {
    if (args.empty() == true) {
        // Default to showing the list of ports
        CommModule::PrintInfo(true);

    } else if (args.size() == 1) {
        if (strcmp(args.front().c_str(), "port") == 0) {
            CommModule::PrintInfo(true);

        } else {
            if (CommModule::isReady() == true) {
                rtp::packet pck;
                std::string msg = "LINK TEST PAYLOAD";

                pck.header_link = RTP_HEADER(rtp::port::LINK, 1, false, false);
                pck.payload_size = msg.length();
                memcpy((char*)pck.payload, msg.c_str(), pck.payload_size);
                pck.address = BASE_STATION_ADDR;

                if (strcmp(args.front().c_str(), "test-tx") == 0) {
                    printf("Placing %u byte packet in TX buffer.\r\n",
                           pck.payload_size);
                    CommModule::send(pck);
                } else if (strcmp(args.front().c_str(), "test-rx") == 0) {
                    printf("Placing %u byte packet in RX buffer.\r\n",
                           pck.payload_size);
                    CommModule::receive(pck);
                } else if (strcmp(args.front().c_str(), "loopback") == 0) {
                    pck.ack = true;
                    pck.subclass = 2;
                    pck.address = LOOPBACK_ADDR;
                    printf(
                        "Placing %u byte packet in TX buffer with ACK set.\r\n",
                        pck.payload_size);
                    CommModule::send(pck);
                } else {
                    show_invalid_args(args.front());
                    return 1;
                }
            } else {
                printf("The radio interface is not ready.\r\n");
            }
        }
    } else if (args.size() == 2) {
        // Default to showing all port info if no specific port number is given
        // for the 'show' option
        if (strcmp(args.front().c_str(), "ports") == 0) {
            if (strcmp(args.at(1).c_str(), "show") == 0) {
                CommModule::PrintInfo(true);
            }
        }
    } else if (args.size() == 3) {
        if (strcmp(args.front().c_str(), "ports") == 0) {
            if (isInt(args.at(2).c_str())) {
                unsigned int portNbr = atoi(args.at(2).c_str());

                if (strcmp(args.at(1).c_str(), "open") == 0) {
                    CommModule::openSocket(portNbr);

                } else if (strcmp(args.at(1).c_str(), "close") == 0) {
                    CommModule::Close(portNbr);
                    printf("Port %u closed.\r\n", portNbr);

                } else if (strcmp(args.at(1).c_str(), "show") == 0) {
                    // Change to show only the requested port's info
                    CommModule::PrintInfo(true);

                } else if (strcmp(args.at(1).c_str(), "reset") == 0) {
                    CommModule::ResetCount(portNbr);
                    printf("Reset packet counts for port %u.\r\n", portNbr);

                } else {
                    show_invalid_args(args);
                    return 1;
                }
            } else {
                show_invalid_args(args.at(2));
                return 1;
            }
        } else {
            show_invalid_args(args.at(2));
            return 1;
        }
    } else {
        show_invalid_args(args);
        return 1;
    }

    return 0;
}

/**
 * Command executor.
 *
 * Much of this taken from `console.c` from the old robot firmware (2011).
 */
void execute_line(char* rawCommand) {
    char* endCmd;
    char* cmds = strtok_r(rawCommand, ";", &endCmd);

    while (cmds != nullptr) {
        uint8_t argc = 0;
        string cmdName = "\0";
        vector<string> args;
        args.reserve(MAX_COMMAND_ARGS);

        char* endArg;
        char* pch = strtok_r(cmds, " ", &endArg);

        while (pch != nullptr) {
            // Check args length
            if (argc > MAX_COMMAND_ARGS) {
                printf("%s\r\n", TOO_MANY_ARGS_MSG.c_str());
                break;
            }

            // Set command name
            if (argc == 0)
                cmdName = string(pch);
            else
                args.push_back(pch);

            argc++;
            pch = strtok_r(nullptr, " ", &endArg);
        }

        if (cmdName.empty() == false) {
            bool commandFound = false;

            for (uint8_t cmdInd = 0; cmdInd < commands.size(); cmdInd++) {
                // check for name match
                if (find(commands[cmdInd].aliases.begin(),
                         commands[cmdInd].aliases.end(),
                         cmdName) != commands[cmdInd].aliases.end()) {
                    commandFound = true;

                    // If the command is desiged to be run every
                    // iteration of the loop, set the handler and
                    // args and flag the loop to execute on each
                    // iteration.
                    if (commands[cmdInd].is_iterative) {
                        iterative_command_state = false;

                        // Sets the current arg count, args, and
                        // command function in fields to be used
                        // in the iterative call.
                        iterative_command_args = args;
                        iterative_command_handler = commands[cmdInd].handler;
                        iterative_command_state = true;
                    }
                    // If the command is not iterative, execute it
                    // once immeidately.
                    else {
                        int exitState = commands[cmdInd].handler(args);
                        if (exitState != 0) {
                            printf("\r\n");
                            cmd_args_t cmdN = {cmdName};
                            cmd_help_detail(cmdN);
                        }
                    }

                    break;
                }
            }
            // if the command wasnt found, print an error
            if (!commandFound) {
                std::size_t pos = COMMAND_NOT_FOUND_MSG.find("%s");

                if (pos == std::string::npos) {  // no format specifier found in
                    // our defined message
                    printf("%s\r\n", COMMAND_NOT_FOUND_MSG.c_str());
                } else {
                    std::string not_found_cmd = COMMAND_NOT_FOUND_MSG;
                    not_found_cmd.replace(pos, 2, cmdName);
                    printf("%s\r\n", not_found_cmd.c_str());
                }
            }
        }

        cmds = strtok_r(nullptr, ";", &endCmd);
        Console::Flush();  // make sure we force everything out of stdout
    }
}

/**
 * Executes iterative commands, and is nonblocking regardless
 * of if an iterative command is not running or not.
 */
void execute_iterative_command() {
    if (iterative_command_state == true) {
        if (Console::IterCmdBreakReq() == true) {
            iterative_command_state = false;

            // reset the flag for receiving a break character in the Console
            // class
            Console::IterCmdBreakReq(false);
        } else {
            iterative_command_handler(iterative_command_args);
        }
    }
}

void show_invalid_args(cmd_args_t& args) {
    printf("Invalid arguments");

    if (args.empty() == false) {
        printf(" ");

        for (unsigned int i = 0; i < args.size() - 1; i++)
            printf("'%s', ", args.at(i).c_str());

        printf("'%s'.", args.back().c_str());
    } else {
        printf(". No arguments given.");
    }

    printf("\r\n");
}

void show_invalid_args(const string& s) {
    printf("Invalid argument '%s'.\r\n", s.c_str());
}
