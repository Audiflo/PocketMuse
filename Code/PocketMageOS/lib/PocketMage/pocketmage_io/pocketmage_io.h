#pragma once
#include <Arduino.h>
#include <vector>

// Split str by delimiter into a vector of substrings.
// Empty tokens are included (e.g. "a||b" -> {"a", "", "b"}).
std::vector<String> splitString(const String& str, char delimiter);

// Join a vector of strings with delimiter between each pair.
String joinString(const std::vector<String>& parts, char delimiter);

// Remove all occurrences of character from str.
String removeChar(String str, char character);

// Parse str as a decimal integer. Returns defaultVal if str is empty,
// contains non-digit characters, or cannot be parsed.
int stringToInt(const String& str, int defaultVal = -1);
