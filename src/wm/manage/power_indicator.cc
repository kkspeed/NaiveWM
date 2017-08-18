#include "wm/manage/power_indicator.h"

#include <cstdio>

#include "base/logging.h"

namespace naive {
namespace extra {

namespace {

template <typename T>
struct DBusTypeConverter {
  static int DBusType;
  static const char* TypeName;
};

template <>
struct DBusTypeConverter<int64_t> {
  static int DBusType;
  static const char* TypeName;
};
int DBusTypeConverter<int64_t>::DBusType = DBUS_TYPE_INT64;
const char* DBusTypeConverter<int64_t>::TypeName = "int64";

template <>
struct DBusTypeConverter<double> {
  static int DBusType;
  static const char* TypeName;
};
int DBusTypeConverter<double>::DBusType = DBUS_TYPE_DOUBLE;
const char* DBusTypeConverter<double>::TypeName = "double";

template <typename T>
T ReadPropertyFromVariant(DBusMessage* reply, DBusError* error) {
  DBusMessageIter iter;
  DBusMessageIter sub;
  T result;

  dbus_message_iter_init(reply, &iter);

  if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter)) {
    dbus_set_error_const(error, "reply_should_be_variant",
                         "This message hasn't a variant response type");
  }

  dbus_message_iter_recurse(&iter, &sub);

  if (DBusTypeConverter<T>::DBusType != dbus_message_iter_get_arg_type(&sub)) {
    // TODO: Set type to T's TypeName.
    dbus_set_error_const(error, "variant_should_be_double",
                         "This variant reply message must have double content");
  }

  dbus_message_iter_get_basic(&sub, &result);
  return result;
}

DBusMessage* create_property_get_message(const char* bus_name,
                                         const char* path,
                                         const char* iface,
                                         const char* propname) {
  DBusMessage* queryMessage = NULL;

  // TODO: Do not hard code this.
  queryMessage = dbus_message_new_method_call(
      bus_name, path, "org.freedesktop.DBus.Properties", "Get");
  dbus_message_append_args(queryMessage, DBUS_TYPE_STRING, &iface,
                           DBUS_TYPE_STRING, &propname, DBUS_TYPE_INVALID);

  return queryMessage;
}

}  // namespace

DBusCon::DBusCon() {
  dbus_error_init(&error_);
  connection_ = dbus_bus_get(DBUS_BUS_SYSTEM, &error_);
  HandleError();
}

void DBusCon::HandleError() {
  if (dbus_error_is_set(&error_))
    TRACE("%s", error_.message);
}

template <typename T>
T DBusCon::GetProperty(const char* bus_name,
                       const char* path,
                       const char* interface,
                       const char* prop) {
  // TODO: Better error handling.
  DBusError myError;
  T result;
  DBusMessage* queryMessage = NULL;
  DBusMessage* replyMessage = NULL;

  dbus_error_init(&myError);

  queryMessage = create_property_get_message(bus_name, path, interface, prop);
  replyMessage = dbus_connection_send_with_reply_and_block(
      connection_, queryMessage, 1000, &myError);
  dbus_message_unref(queryMessage);
  if (dbus_error_is_set(&myError)) {
    dbus_move_error(&myError, &error_);
    return 0;
  }

  result = ReadPropertyFromVariant<T>(replyMessage, &myError);
  if (dbus_error_is_set(&myError)) {
    dbus_move_error(&myError, &error_);
    return 0;
  }

  dbus_message_unref(replyMessage);

  return result;
}

int64_t DBusCon::GetRemainingTime() {
  // TODO: do not hard code this interface.
  int64_t result = GetProperty<int64_t>(
      "org.freedesktop.UPower", "/org/freedesktop/UPower/devices/battery_BAT0",
      "org.freedesktop.UPower.Device", "TimeToEmpty");
  HandleError();
  return result;
}

double DBusCon::GetPercentage() {
  // TODO: do not hard code this interface.
  double result = GetProperty<double>(
      "org.freedesktop.UPower", "/org/freedesktop/UPower/devices/battery_BAT0",
      "org.freedesktop.UPower.Device", "Percentage");
  HandleError();
  return result;
}

PowerIndicator::PowerIndicator(int32_t x,
                               int32_t y,
                               int32_t width,
                               int32_t height)
    : TextView(x, y, width, height) {
  SetTextColor(0xFF00FFFF);
  SetTextSize(16);
}

void PowerIndicator::OnDrawFrame() {
  TextView::OnDrawFrame();

  if (frame_ == 0)
    UpdatePowerInfo();
  frame_ = (frame_ + 1) % 3600;
}

void PowerIndicator::UpdatePowerInfo() {
  int64_t time = dbus_.GetRemainingTime();
  double remaining = dbus_.GetPercentage();

  int hour = static_cast<int>(time / 3600);
  int minute = static_cast<int>((time % 3600) / 60);
  char buffer[80];
  // TODO: Use better method to decide whether it's charging (getstatus).
  if (hour == 0 && minute == 0)
    std::sprintf(buffer, "Battery: charing (%.2lf%%)", remaining);
  else
    std::sprintf(buffer, "Battery: %2d:%.2d (%.2lf%%)", hour, minute,
                 remaining);
  SetText(std::string(buffer));
  if (hour == 0 && minute <= 30 && minute > 0 && remaining <= 15)
    SetBackgroundColor(0xFFDD0000);
  else
    SetBackgroundColor(0xFF000000);
}

}  // namespace extra
}  // namespace naive
