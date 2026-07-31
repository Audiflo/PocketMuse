#include "pocketmage_io.h"

std::vector<String> splitString(const String& str, char delimiter) {
  std::vector<String> result;
  int start = 0;
  while (true) {
    int end = str.indexOf(delimiter, start);
    if (end == -1) {
      result.push_back(str.substring(start));
      break;
    }
    result.push_back(str.substring(start, end));
    start = end + 1;
  }
  return result;
}

String joinString(const std::vector<String>& parts, char delimiter) {
  String result;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) result += delimiter;
    result += parts[i];
  }
  return result;
}

String removeChar(String str, char character) {
  String result;
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] != character) {
      result += str[i];
    }
  }
  return result;
}

int stringToInt(const String& str, int defaultVal) {
  String s = str;
  s.trim();
  if (s.length() == 0) return defaultVal;
  unsigned int start = (s.charAt(0) == '+' || s.charAt(0) == '-') ? 1 : 0;
  for (unsigned int i = start; i < s.length(); i++) {
    if (!isDigit(s.charAt(i))) return defaultVal;
  }
  return s.toInt();
}
