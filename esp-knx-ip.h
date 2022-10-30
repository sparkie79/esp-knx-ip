/**
 * esp-knx-ip library for KNX/IP communication on an ESP8266
 * Author: Nico Weichbrodt <envy>
 * License: MIT
 */

#ifndef ESP_KNX_IP_H
#define ESP_KNX_IP_H

/**
 * CONFIG
 * All MAX_ values must not exceed 255 (1 byte, except MAC_CONFIG_SPACE which can go up to 2 bytes, so 0xffff in theory) and must not be negative!
 * Config space is restriced by EEPROM_SIZE (default 1024).
 * Required EEPROM size is 8 + MAX_GA_CALLBACKS * 3 + 2 + MAX_CONFIG_SPACE which is 552 by default
 */
#define EEPROM_SIZE               1024 // [Default 1024]
#define MAX_CALLBACK_ASSIGNMENTS  10 // [Default 10] Maximum number of group address callbacks that can be stored
#define MAX_CALLBACKS             10 // [Default 10] Maximum number of callbacks that can be stored
#define MAX_CONFIGS               20 // [Default 20] Maximum number of config items that can be stored
#define MAX_CONFIG_SPACE          0x0200 // [Default 0x0200] Maximum number of bytes that can be stored for custom config

#define MAX_FEEDBACKS             20 // [Default 20] Maximum number of feedbacks that can be shown

// Callbacks
#define ALLOW_MULTIPLE_CALLBACKS_PER_ADDRESS  0 // [Default 0] Set to 1 to always test all assigned callbacks. This allows for multiple callbacks being assigned to the same address. If disabled, only the first assigned will be called.

// Webserver related
#define USE_BOOTSTRAP             1 // [Default 1] Set to 1 to enable use of bootstrap CSS for nicer webconfig. CSS is loaded from bootstrapcdn.com. Set to 0 to disable
#define ROOT_PREFIX               ""  // [Default ""] This gets prepended to all webserver paths, default is empty string "". Set this to "/knx" if you want the config to be available on http://<ip>/knx
#define DISABLE_EEPROM_BUTTONS    0 // [Default 0] Set to 1 to disable the EEPROM buttons in the web ui.
#define DISABLE_REBOOT_BUTTON     0 // [Default 0] Set to 1 to disable the reboot button in the web ui.
#define DISABLE_RESTORE_BUTTON    0 // [Default 0] Set to 1 to disable the "restore defaults" button in the web ui.
#define DISABLE_SWUPDATE_BUTTON   0 // [Default 0] Set to 1 to disable the "SW Update" button in the web ui.

// These values normally don't need adjustment
#ifndef MULTICAST_PORT
#define MULTICAST_PORT            3671 // [Default 3671]
#endif
#ifndef MULTICAST_IP
#define MULTICAST_IP              IPAddress(224, 0, 23, 12) // [Default IPAddress(224, 0, 23, 12)]
#endif
#define SEND_CHECKSUM             0

// Uncomment to enable printing out debug messages.
// #define ESP_KNX_DEBUG
/**
 * END CONFIG
 */

#include "Arduino.h"

#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif

#include <EEPROM.h>
#include <WiFiUdp.h>


#include "DPT.h"
#include "esp-knx-types.h"



#define EEPROM_MAGIC (0xDEADBEEF00000000 + (MAX_CONFIG_SPACE) + (MAX_CALLBACK_ASSIGNMENTS << 16) + (MAX_CALLBACKS << 8))

// Define where debug output will be printed.
#ifndef DEBUG_PRINTER
#define DEBUG_PRINTER Serial
#endif

// Setup debug printing macros.
#ifdef ESP_KNX_DEBUG
  #define DEBUG_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
  #define DEBUG_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#else
  #define DEBUG_PRINT(...) {}
  #define DEBUG_PRINTLN(...) {}
#endif

#define __ROOT_PATH       ROOT_PREFIX"/"
#define __REGISTER_PATH   ROOT_PREFIX"/register"
#define __DELETE_PATH     ROOT_PREFIX"/delete"
#define __PHYS_PATH       ROOT_PREFIX"/phys"
#define __EEPROM_PATH     ROOT_PREFIX"/eeprom"
#define __CONFIG_PATH     ROOT_PREFIX"/config"
#define __FEEDBACK_PATH   ROOT_PREFIX"/feedback"
#define __RESTORE_PATH    ROOT_PREFIX"/restore"
#define __REBOOT_PATH     ROOT_PREFIX"/reboot"
#define __SWUPDATE_PATH   ROOT_PREFIX"/swupdate"



class ESPKNXIP {
  public:
    ESPKNXIP();
    void load();
    void start();
    // title for web page
    void set_title(String t) { title = t; }
    void loop();

    void save_to_eeprom();
    void restore_from_eeprom();

    callback_id_t callback_register(String name, callback_fptr_t cb, void *arg = nullptr, enable_condition_t cond = nullptr);
    void          callback_assign(callback_id_t id, address_t val);

    void          physical_address_set(address_t const &addr);
    address_t     physical_address_get();

  // Configuration functions
    config_id_t   config_register_string(String name, uint8_t len, String _default, enable_condition_t cond = nullptr);
    config_id_t   config_register_int(String name, int32_t _default, enable_condition_t cond = nullptr);
    config_id_t   config_register_bool(String name, bool _default, enable_condition_t cond = nullptr);
    config_id_t   config_register_options(String name, option_entry_t *options, uint8_t _default, enable_condition_t cond = nullptr);
    config_id_t   config_register_ga(String name, enable_condition_t cond = nullptr);

    String        config_get_string(config_id_t id);
    int32_t       config_get_int(config_id_t id);
    bool          config_get_bool(config_id_t id);
    uint8_t       config_get_options(config_id_t id);
    address_t     config_get_ga(config_id_t id);

    void          config_set_string(config_id_t id, String val);
    void          config_set_int(config_id_t id, int32_t val);
    void          config_set_bool(config_id_t, bool val);
    void          config_set_options(config_id_t id, uint8_t val);
    void          config_set_ga(config_id_t id, address_t const &val);

    // Feedback functions
    feedback_id_t feedback_register_int(String name, int32_t *value, enable_condition_t cond = nullptr);
    feedback_id_t feedback_register_float(String name, float *value, uint8_t precision = 2, enable_condition_t cond = nullptr);
    feedback_id_t feedback_register_bool(String name, bool *value, enable_condition_t cond = nullptr);
    feedback_id_t feedback_register_action(String name, feedback_action_fptr_t value, void *arg = nullptr, enable_condition_t = nullptr);

    // Send functions
    void send(address_t const &receiver, knx_command_type_t ct, uint8_t data_len, uint8_t *data);

    void send_1bit(address_t const &receiver, knx_command_type_t ct, uint8_t bit);
    void send_2bit(address_t const &receiver, knx_command_type_t ct, uint8_t twobit);
    void send_4bit(address_t const &receiver, knx_command_type_t ct, uint8_t fourbit);
    void send_1byte_int(address_t const &receiver, knx_command_type_t ct, int8_t val);
    void send_1byte_uint(address_t const &receiver, knx_command_type_t ct, uint8_t val);
    void send_2byte_int(address_t const &receiver, knx_command_type_t ct, int16_t val);
    void send_2byte_uint(address_t const &receiver, knx_command_type_t ct, uint16_t val);
    void send_2byte_float(address_t const &receiver, knx_command_type_t ct, float val);
    void send_3byte_time(address_t const &receiver, knx_command_type_t ct, uint8_t weekday, uint8_t hours, uint8_t minutes, uint8_t seconds);
    void send_3byte_time(address_t const &receiver, knx_command_type_t ct, time_of_day_t const &time) { send_3byte_time(receiver, ct, time.weekday, time.hours, time.minutes, time.seconds); }
    void send_3byte_date(address_t const &receiver, knx_command_type_t ct, uint8_t day, uint8_t month, uint8_t year);
    void send_3byte_date(address_t const &receiver, knx_command_type_t ct, date_t const &date) { send_3byte_date(receiver, ct, date.day, date.month, date.year); }
    void send_3byte_color(address_t const &receiver, knx_command_type_t ct, uint8_t red, uint8_t green, uint8_t blue);
    void send_3byte_color(address_t const &receiver, knx_command_type_t ct, color_t const &color) { send_3byte_color(receiver, ct, color.red, color.green, color.blue); }
    void send_4byte_int(address_t const &receiver, knx_command_type_t ct, int32_t val);
    void send_4byte_uint(address_t const &receiver, knx_command_type_t ct, uint32_t val);
    void send_4byte_float(address_t const &receiver, knx_command_type_t ct, float val);
    void send_14byte_string(address_t const &receiver, knx_command_type_t ct, const char *val);

    void write_1bit(address_t const &receiver, uint8_t bit) { send_1bit(receiver, KNX_CT_WRITE, bit); }
    void write_2bit(address_t const &receiver, uint8_t twobit) { send_2bit(receiver, KNX_CT_WRITE, twobit); }
    void write_4bit(address_t const &receiver, uint8_t fourbit) { send_4bit(receiver, KNX_CT_WRITE, fourbit); }
    void write_1byte_int(address_t const &receiver, int8_t val) { send_1byte_int(receiver, KNX_CT_WRITE, val); }
    void write_1byte_uint(address_t const &receiver, uint8_t val) { send_1byte_uint(receiver, KNX_CT_WRITE, val); }
    void write_2byte_int(address_t const &receiver, int16_t val) { send_2byte_int(receiver, KNX_CT_WRITE, val); }
    void write_2byte_uint(address_t const &receiver, uint16_t val) { send_2byte_uint(receiver, KNX_CT_WRITE, val); }
    void write_2byte_float(address_t const &receiver, float val) { send_2byte_float(receiver, KNX_CT_WRITE, val); }
    void write_3byte_time(address_t const &receiver, uint8_t weekday, uint8_t hours, uint8_t minutes, uint8_t seconds) { send_3byte_time(receiver, KNX_CT_WRITE, weekday, hours, minutes, seconds); }
    void write_3byte_time(address_t const &receiver, time_of_day_t const &time) { send_3byte_time(receiver, KNX_CT_WRITE, time.weekday, time.hours, time.minutes, time.seconds); }
    void write_3byte_date(address_t const &receiver, uint8_t day, uint8_t month, uint8_t year) { send_3byte_date(receiver, KNX_CT_WRITE, day, month, year); }
    void write_3byte_date(address_t const &receiver, date_t const &date) { send_3byte_date(receiver, KNX_CT_WRITE, date.day, date.month, date.year); }
    void write_3byte_color(address_t const &receiver, uint8_t red, uint8_t green, uint8_t blue) { send_3byte_color(receiver, KNX_CT_WRITE, red, green, blue); }
    void write_3byte_color(address_t const &receiver, color_t const &color) { send_3byte_color(receiver, KNX_CT_WRITE, color); }
    void write_4byte_int(address_t const &receiver, int32_t val) { send_4byte_int(receiver, KNX_CT_WRITE, val); }
    void write_4byte_uint(address_t const &receiver, uint32_t val) { send_4byte_uint(receiver, KNX_CT_WRITE, val); }
    void write_4byte_float(address_t const &receiver, float val) { send_4byte_float(receiver, KNX_CT_WRITE, val); }
    void write_14byte_string(address_t const &receiver, const char *val) { send_14byte_string(receiver, KNX_CT_WRITE, val); }

    void answer_1bit(address_t const &receiver, uint8_t bit) { send_1bit(receiver, KNX_CT_ANSWER, bit); }
    void answer_2bit(address_t const &receiver, uint8_t twobit) { send_2bit(receiver, KNX_CT_ANSWER, twobit); }
    void answer_4bit(address_t const &receiver, uint8_t fourbit) { send_4bit(receiver, KNX_CT_ANSWER, fourbit); }
    void answer_1byte_int(address_t const &receiver, int8_t val) { send_1byte_int(receiver, KNX_CT_ANSWER, val); }
    void answer_1byte_uint(address_t const &receiver, uint8_t val) { send_1byte_uint(receiver, KNX_CT_ANSWER, val); }
    void answer_2byte_int(address_t const &receiver, int16_t val) { send_2byte_int(receiver, KNX_CT_ANSWER, val); }
    void answer_2byte_uint(address_t const &receiver, uint16_t val) { send_2byte_uint(receiver, KNX_CT_ANSWER, val); }
    void answer_2byte_float(address_t const &receiver, float val) { send_2byte_float(receiver, KNX_CT_ANSWER, val); }
    void answer_3byte_time(address_t const &receiver, uint8_t weekday, uint8_t hours, uint8_t minutes, uint8_t seconds) { send_3byte_time(receiver, KNX_CT_ANSWER, weekday, hours, minutes, seconds); }
    void answer_3byte_time(address_t const &receiver, time_of_day_t const &time) { send_3byte_time(receiver, KNX_CT_ANSWER, time.weekday, time.hours, time.minutes, time.seconds); }
    void answer_3byte_date(address_t const &receiver, uint8_t day, uint8_t month, uint8_t year) { send_3byte_date(receiver, KNX_CT_ANSWER, day, month, year); }
    void answer_3byte_date(address_t const &receiver, date_t const &date) { send_3byte_date(receiver, KNX_CT_ANSWER, date.day, date.month, date.year); }
    void answer_3byte_color(address_t const &receiver, uint8_t red, uint8_t green, uint8_t blue) { send_3byte_color(receiver, KNX_CT_ANSWER, red, green, blue); }
    void answer_3byte_color(address_t const &receiver, color_t const &color) { send_3byte_color(receiver, KNX_CT_ANSWER, color); }
    void answer_4byte_int(address_t const &receiver, int32_t val) { send_4byte_int(receiver, KNX_CT_ANSWER, val); }
    void answer_4byte_uint(address_t const &receiver, uint32_t val) { send_4byte_uint(receiver, KNX_CT_ANSWER, val); }
    void answer_4byte_float(address_t const &receiver, float val) { send_4byte_float(receiver, KNX_CT_ANSWER, val);}
    void answer_14byte_string(address_t const &receiver, const char *val) { send_14byte_string(receiver, KNX_CT_ANSWER, val); }

    bool          data_to_bool(uint8_t *data);
    int8_t        data_to_1byte_int(uint8_t *data);
    uint8_t       data_to_1byte_uint(uint8_t *data);
    int16_t       data_to_2byte_int(uint8_t *data);
    uint16_t      data_to_2byte_uint(uint8_t *data);
    float         data_to_2byte_float(uint8_t *data);
    color_t       data_to_3byte_color(uint8_t *data);
    time_of_day_t data_to_3byte_time(uint8_t *data);
    date_t        data_to_3byte_data(uint8_t *data);
    int32_t       data_to_4byte_int(uint8_t *data);
    uint32_t      data_to_4byte_uint(uint8_t *data);
    float         data_to_4byte_float(uint8_t *data);

    static address_t GA_to_address(uint8_t area, uint8_t line, uint8_t member)
    {
      // Yes, the order is correct, see the struct definition above
      address_t tmp = {.ga={line, area, member}};
      return tmp;
    }

    static address_t PA_to_address(uint8_t area, uint8_t line, uint8_t member)
    {
      // Yes, the order is correct, see the struct definition above
      address_t tmp = {.pa={line, area, member}};
      return tmp;
    }

  private:
    void __start();
    void __loop_knx();

    void __config_set_flags(config_id_t id, config_flags_t flags);

    void __config_set_string(config_id_t id, String &val);
    void __config_set_int(config_id_t id, int32_t val);
    void __config_set_bool(config_id_t id, bool val);
    void __config_set_options(config_id_t id, uint8_t val);
    void __config_set_ga(config_id_t id, address_t const &val);

    callback_assignment_id_t __callback_register_assignment(address_t address, callback_id_t id);
    void __callback_delete_assignment(callback_assignment_id_t id);
    
    String title;

    address_t physaddr;
    WiFiUDP udp;

    callback_assignment_id_t registered_callback_assignments;
    callback_assignment_t callback_assignments[MAX_CALLBACK_ASSIGNMENTS];

    callback_id_t registered_callbacks;
    callback_t callbacks[MAX_CALLBACKS];

    config_id_t registered_configs;
    uint8_t custom_config_data[MAX_CONFIG_SPACE];
    uint8_t custom_config_default_data[MAX_CONFIG_SPACE];
    config_t custom_configs[MAX_CONFIGS];

    feedback_id_t registered_feedbacks;
    feedback_t feedbacks[MAX_FEEDBACKS];

    uint16_t __ntohs(uint16_t);
};

// Global "singleton" object
extern ESPKNXIP knx;

#endif
