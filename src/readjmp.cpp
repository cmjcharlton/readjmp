// [[Rcpp::plugins(cpp23)]]

#include <Rcpp.h>

#include <bit>
#include <bitset>
#include <chrono>
#include <cstdint>
#include <format>
#include <fstream>
#include <istream>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "zlib.h"

typedef std::bitset<80> Float80;

enum class encoding { UTF8 = 0, Native = 1, XMacRoman = 2, Unknown = 3 };

#ifndef __cpp_lib_byteswap
// Replacement function from https://en.cppreference.com/cpp/numeric/byteswap
template<std::integral T>
constexpr T byteswap(T value) noexcept {
  auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
  std::ranges::reverse(value_representation);
  return std::bit_cast<T>(value_representation);
}
#endif

static void skipBytes(std::istream& fs, std::size_t off) {
  fs.seekg(off, std::ios::cur);
}

template <typename T>
[[nodiscard]] T readValue(std::istream& fs, std::endian byteorder) {
  T result{};
  fs.read((char*)&result, sizeof(T));
  if (std::endian::native != byteorder) {
    if constexpr (std::is_same_v<T, float>) {
#ifndef __cpp_lib_byteswap
      result = std::bit_cast<float>(byteswap(std::bit_cast<std::uint32_t>(result)));
#else
      result = std::bit_cast<float>(std::byteswap(std::bit_cast<std::uint32_t>(result)));
#endif
    }
    else if constexpr (std::is_same_v<T, double>) {
#ifndef __cpp_lib_byteswap
      result = std::bit_cast<double>(byteswap(std::bit_cast<std::uint64_t>(result)));
#else
      result = std::bit_cast<double>(std::byteswap(std::bit_cast<std::uint64_t>(result)));
#endif
    }
    else {
#ifndef __cpp_lib_byteswap
      result = byteswap(result);
#else
      result = std::byteswap(result);
#endif
    }
  }
  return result;
}

template <>
[[nodiscard]] Float80 readValue<Float80>(std::istream& fs, std::endian byteorder) {
  Float80 result{};
  std::array<std::uint8_t, 10> bytes{};
  fs.read((char*)bytes.data(), bytes.size());
  if (std::endian::native != byteorder) {
    std::uint8_t start = 72;
    for (std::uint8_t i = 0; i < 10; ++i) {
      std::bitset<8> byte(bytes[i]);
      for (std::uint8_t j = 0; j < 8; ++j) {
        result[start + j] = byte[j];
      }
      start -= 8;
    }
  }
  else {
    std::uint8_t start = 0;
    for (std::uint8_t i = 0; i < 10; ++i) {
      std::bitset<8> byte(bytes[i]);
      for (std::uint8_t j = 0; j < 8; ++j) {
        result[start + j] = byte[j];
      }
      start += 8;
    }
  }
  return result;
}

// Note:
// Replaced non-breaking space (00A0) with space (0020)
// Replaced soft-hyphen (00AD) with hyphen (002D)
static constexpr std::string_view ansichars[] {
  "\u20AC", "\u0020", "\u201A", "\u0192", "\u201E", "\u2026", "\u2020", "\u2021",
  "\u02C6", "\u2030", "\u0160", "\u2039", "\u0152", "\u0020", "\u017D", "\u0020",
  "\u0020", "\u2018", "\u2019", "\u201C", "\u201D", "\u2022", "\u2013", "\u2014",
  "\u02DC", "\u2122", "\u0161", "\u203A", "\u0153", "\u0020", "\u017E", "\u0178",
  "\u0020", "\u00A1", "\u00A2", "\u00A3", "\u00A4", "\u00A5", "\u00A6", "\u00A7",
  "\u00A8", "\u00A9", "\u00AA", "\u00AB", "\u00AC", "\u0045", "\u00AE", "\u00AF",
  "\u00B0", "\u00B1", "\u00B2", "\u00B3", "\u00B4", "\u00B5", "\u00B6", "\u00B7",
  "\u00B8", "\u00B9", "\u00BA", "\u00BB", "\u00BC", "\u00BD", "\u00BE", "\u00BF",
  "\u00C0", "\u00C1", "\u00C2", "\u00C3", "\u00C4", "\u00C5", "\u00C6", "\u00C7",
  "\u00C8", "\u00C9", "\u00CA", "\u00CB", "\u00CC", "\u00CD", "\u00CE", "\u00CF",
  "\u00D0", "\u00D1", "\u00D2", "\u00D3", "\u00D4", "\u00D5", "\u00D6", "\u00D7",
  "\u00D8", "\u00D9", "\u00DA", "\u00DB", "\u00DC", "\u00DD", "\u00DE", "\u00DF",
  "\u00E0", "\u00E1", "\u00E2", "\u00E3", "\u00E4", "\u00E5", "\u00E6", "\u00E7",
  "\u00E8", "\u00E9", "\u00EA", "\u00EB", "\u00EC", "\u00ED", "\u00EE", "\u00EF",
  "\u00F0", "\u00F1", "\u00F2", "\u00F3", "\u00F4", "\u00F5", "\u00F6", "\u00F7",
  "\u00F8", "\u00F9", "\u00FA", "\u00FB", "\u00FC", "\u00FD", "\u00FE", "\u00FF"
};

static constexpr std::string_view macchars[] {
  "\u00C4", "\u00C5", "\u00C7", "\u00C9", "\u00D1", "\u00D6", "\u00DC", "\u00E1",
  "\u00E0", "\u00E2", "\u00E4", "\u00E3", "\u00E5", "\u00E7", "\u00E9", "\u00E8",
  "\u00EA", "\u00EB", "\u00EC", "\u00ED", "\u00EE", "\u00EF", "\u00F1", "\u00F3",
  "\u00F2", "\u00F4", "\u00F6", "\u00F5", "\u00FA", "\u00F9", "\u00FB", "\u00FC",
  "\u2020", "\u00B0", "\u00A2", "\u00A3", "\u00A7", "\u2022", "\u00B6", "\u00DF",
  "\u00AE", "\u00A9", "\u2122", "\u00B4", "\u00A8", "\u2260", "\u00C6", "\u00D8",
  "\u221E", "\u00B1", "\u2264", "\u2265", "\u00A5", "\u00B5", "\u2202", "\u2211",
  "\u220F", "\u03C0", "\u222B", "\u00AA", "\u00BA", "\u03A9", "\u00E6", "\u00F8",
  "\u00BF", "\u00A1", "\u00AC", "\u221A", "\u0192", "\u2248", "\u2206", "\u00AB",
  "\u00BB", "\u2026", "\u00A0", "\u00C0", "\u00C3", "\u00D5", "\u0152", "\u0153",
  "\u2013", "\u2014", "\u201C", "\u201D", "\u2018", "\u2019", "\u00F7", "\u25CA",
  "\u00FF", "\u0178", "\u2044", "\u20AC", "\u2039", "\u203A", "\uFB01", "\uFB02",
  "\u2021", "\u00B7", "\u201A", "\u201E", "\u2030", "\u00C2", "\u00CA", "\u00C1",
  "\u00CB", "\u00C8", "\u00CD", "\u00CE", "\u00CF", "\u00CC", "\u00D3", "\u00D4",
  "\uF8FF", "\u00D2", "\u00DA", "\u00DB", "\u00D9", "\u0131", "\u02C6", "\u02DC",
  "\u00AF", "\u02D8", "\u02D9", "\u02DA", "\u00B8", "\u02DD", "\u02DB", "\u02C7"
};

// Convert encoding to UTF-8 if required
[[nodiscard]] static std::string Enc2UTF8(const std::string &input, encoding enc) {
  std::string result{};
  if (enc == encoding::UTF8) {
    result = input;
  }
  else {
    for (std::uint8_t c : input) {
      if (c < 128) {
        result += c;
      }
      else {
        if (enc == encoding::XMacRoman) {
          result += macchars[c - 128];
        }
        else {
          result += ansichars[c - 128];
        }
      }
    }
  }
  return result;
}

[[nodiscard]] static std::string readString(std::istream& fs, std::size_t len) {
  std::string result(len, ' ');
  fs.read((char*)result.data(), result.size());
  return result;
}

[[nodiscard]] static std::string readCString(std::istream& fs, std::size_t len) {
  std::string result(len, ' ');
  fs.read((char*)result.data(), result.size());
  auto nullpos = result.find('\0');
  if (nullpos != std::string::npos) {
    result.erase(nullpos);
  }
  return result;
}

template <typename T>
[[nodiscard]] static Rcpp::IntegerVector readIntegerColumn(std::istream& fs, std::uint64_t nrec, std::endian byteorder) {
  Rcpp::IntegerVector result(nrec);
  for (std::uint64_t i = 0; i < nrec; ++i) {
    T val = readValue<T>(fs, byteorder);
    if (val <= std::numeric_limits<T>::min() + 1) {
      result[i] = NA_INTEGER;
    }
    else {
      result[i] = val;
    }
  }
  return result;
}

template <typename T>
[[nodiscard]] static Rcpp::NumericVector readFloatColumn(std::istream& fs, std::uint64_t nrec, std::endian byteorder) {
  Rcpp::NumericVector result(nrec);
  for (std::uint64_t i = 0; i < nrec; ++i) {
    if constexpr (std::is_same_v<T, Float80>) {
      if constexpr (sizeof(long double) == 10) {
        long double val = readValue<long double>(fs, byteorder);
        if (!std::isfinite(val)) {
          result[i] = NA_REAL;
        }
        else {
          result[i] = static_cast<double>(val);
        }
      }
      else {
        // Convert extended-precision to double
        // see https://en.wikipedia.org/wiki/Extended_precision for layout details
        // FIXME: This could probably be done by copying bits directly, but then
        // special handling for rounding would be required
        Float80 val = readValue<Float80>(fs, byteorder);
        std::bitset<64> fraction{};
        for (std::uint8_t j = 0; j < 63; ++j) {
          fraction[j] = val[j];
        }
        std::uint8_t intpart = val[63];
        std::bitset<15> exponent{};
        for (std::uint8_t j = 0; j < 15; ++j) {
          exponent[j] = val[64 + j];
        }
        
        double signpart = (val[79] == 0) ? 1.0 : -1.0;
        std::uint64_t f = fraction.to_ullong();
        std::uint32_t e = exponent.to_ulong();
        
        double value{};
        if (e == 0) { // All zero
          if (intpart == 0 && f == 0) {
            value = 0;
          }
          else {
            double fracpart = static_cast<double>(f) / (std::uint64_t(1) << 63);
            double exppart = std::pow(2, e - 16382.0);
            value = signpart * (intpart + fracpart) * exppart;
          }
        }
        else if (e == 32767) { // All one
          if (intpart == 0) {
            if (f == 0) {
              value = std::numeric_limits<double>::infinity();
            }
            else {
              value = std::numeric_limits<double>::signaling_NaN();
            }
          }
          else {
            if (f == 0) {
              value = std::numeric_limits<double>::infinity();
            }
            else {
              value = std::numeric_limits<double>::quiet_NaN();
            }
          }
        }
        else {
          if (intpart != 1) {
            // Unnormal
          }
          double fracpart = static_cast<double>(f) / (std::uint64_t(1) << 63);
          double exppart = std::pow(2, e - 16383.0);
          value = signpart * (intpart + fracpart) * exppart;
        }
        if (!std::isfinite(value)) {
          result[i] = NA_REAL;
        }
        else {
          result[i] = value;
        }
      }
    }
    else {
      T val = readValue<T>(fs, byteorder);
      if (!std::isfinite(val)) {
        result[i] = NA_REAL;
      }
      else {
        result[i] = val;
      }
    }
  }
  return result;
}

[[nodiscard]] static Rcpp::NumericVector readDateColumn(std::istream& fs, std::uint64_t nrec, std::endian byteorder) {
  Rcpp::DatetimeVector result(nrec);
  for (std::uint64_t i = 0; i < nrec; ++i) {
    double val = readValue<double>(fs, byteorder);
    if (!std::isfinite(val)) {
      result[i] = NA_REAL;
    }
    else {
      result[i] = val - 2082844800; // Adjust from Mac epoch of 01/01/1904
    }
  }
  return result;
}

template <typename T>
[[nodiscard]] static Rcpp::StringVector readFixedLengthStringColumn(std::istream& fs, std::uint64_t nrec, std::endian byteorder, T width, encoding enc) {
  Rcpp::StringVector result(nrec);
  for (std::uint64_t i = 0; i < nrec; ++i) {
    T length = readValue<T>(fs, byteorder);
    std::string val = readString(fs, std::max(length, width));
    val.resize(length);
    result[i] = Enc2UTF8(val, enc);
  }
  return result;
}

template <typename T>
[[nodiscard]] static Rcpp::StringVector readVariableLengthStringColumn(std::istream& fs, std::uint64_t nrec, std::endian byteorder, encoding enc) {
  [[maybe_unused]] std::uint32_t maxcollen = readValue<std::uint32_t>(fs, byteorder); // max width
  std::vector<T> lens(nrec);
  for (std::uint64_t i = 0; i < nrec; ++i) {
    lens[i] = readValue<T>(fs, byteorder);
  }
  Rcpp::StringVector result(nrec);
  for (std::uint64_t i = 0; i < nrec; ++i) {
    result[i] = Enc2UTF8(readString(fs, lens[i]), enc);
  }
  return result;
}

template <typename T>
[[nodiscard]] static Rcpp::StringVector readCStringColumn(std::istream& fs, std::uint64_t nrec, T width, encoding enc) {
  Rcpp::StringVector result(nrec);
  for (std::uint64_t i = 0; i < nrec; ++i) {
    result[i] = Enc2UTF8(readCString(fs, width), enc);
  }
  return result;
}

template <typename T>
[[nodiscard]] static Rcpp::IntegerVector readCategoricalColumn(std::istream& fs, std::uint64_t nrec, std::uint64_t ncat, std::endian byteorder, encoding enc) {
  Rcpp::IntegerVector rowvalues(nrec);
  for (std::size_t i = 0; i < nrec; ++i) {
    T value = readValue<T>(fs, byteorder);
    if (value == 0) {
      rowvalues[i] = NA_INTEGER; // Treat category zero as missing
    }
    else {
      rowvalues[i] = value;
    }
  }

  std::uint8_t lenbytes = readValue<std::uint8_t>(fs, byteorder);
  Rcpp::CharacterVector categories(ncat - 1);
  for (std::size_t i = 0; i < ncat - 1; ++i) {
    if (lenbytes == 1) {
      std::uint8_t catlen = readValue<std::uint8_t>(fs, byteorder);
      categories[i] = Enc2UTF8(readString(fs, catlen), enc);
    }
    else if (lenbytes == 2) {
      std::uint16_t catlen = readValue<std::uint16_t>(fs, byteorder);
      categories[i] = Enc2UTF8(readString(fs, catlen), enc);
    }
    else if (lenbytes == 4) {
      std::uint32_t catlen = readValue<std::uint32_t>(fs, byteorder);
      categories[i] = Enc2UTF8(readString(fs, catlen), enc);
    }
    else {
      Rcpp::warning("Unexpected size %i", lenbytes);
    }
  }
  rowvalues.attr("class") = "factor";
  rowvalues.attr("levels") = categories;
  return rowvalues;
}

template <typename T>
[[nodiscard]] static Rcpp::IntegerVector readRowStateColumn(std::istream& fs, std::uint64_t nrec, std::endian byteorder) {
  // Contains markers, colours, etc
  // See https://www.jmp.com/support/help/en/18.2/index.shtml#page/jmp/row-states.shtml
  // TODO: Convert these values into something more useful
  Rcpp::IntegerVector result(nrec);
  for (std::uint64_t i = 0; i < nrec; ++i) {
    T val = readValue<T>(fs, byteorder);
    result[i] = val;
  }
  return result;
}

[[nodiscard]] static Rcpp::RObject readColumn(std::uint32_t version, std::istream& fs, std::endian byteorder, encoding enc, std::uint64_t nrec, std::uint8_t datatype, std::uint8_t fdata, std::uint8_t recwidth1, std::uint8_t recwidth2) {
  Rcpp::RObject result;
  if (datatype == 1) { // Floating point
    if (fdata >= 101 && fdata <= 138) { // Date/Time values
      result = readDateColumn(fs, nrec, byteorder);
    }
    else {
      switch (recwidth2) {
      case 1: // byte
        result = readIntegerColumn<std::int8_t>(fs, nrec, byteorder);
        break;
      case 2: // short
        result = readIntegerColumn<std::int16_t>(fs, nrec, byteorder);
        break;
      case 4: // int
        result = readIntegerColumn<std::int32_t>(fs, nrec, byteorder);
        break;
      case 8: // double-precision float (version >= 3)
        result = readFloatColumn<double>(fs, nrec, byteorder);
        break;
      case 10: // extended-precision float (version 1)
        result = readFloatColumn<Float80>(fs, nrec, byteorder);
        break;
      default:
        Rcpp::warning("Unhandled size: %i", recwidth2);
      }
    }
  }
  else if (datatype == 2) { // String
    if (recwidth1 == 0) {
      if (version < 7) {
        result = readFixedLengthStringColumn<std::uint8_t>(fs, nrec, byteorder, recwidth2 - 1, enc);
      }
      else if (version == 7) {
        std::uint64_t collenbytes = readValue<std::uint64_t>(fs, byteorder); // block size
        auto colstart = fs.tellg();
        std::uint8_t collenbytewidth = readValue<std::uint8_t>(fs, byteorder); // int width in bytes
        switch (collenbytewidth) {
        case 1:
          result = readVariableLengthStringColumn<std::uint8_t>(fs, nrec, byteorder, enc);
          break;
        case 2:
          result = readVariableLengthStringColumn<std::uint16_t>(fs, nrec, byteorder, enc);
          break;
        case 4:
          result = readVariableLengthStringColumn<std::uint32_t>(fs, nrec, byteorder, enc);
          break;
        default:
          Rcpp::warning("Unhandled int size: %i", collenbytewidth);
        }
        auto colend = fs.tellg();
        std::uint64_t sizeread = colend - colstart;
        if (collenbytes != sizeread) {
          Rcpp::warning("Unexpected number of bytes read expected: %i got: %i", collenbytes, sizeread);
        }
      }
      else { // version 8
        auto colstart = fs.tellg();
        std::uint64_t collenbytes = readValue<std::uint64_t>(fs, byteorder); // block size
        std::uint8_t sig1 = readValue<std::uint8_t>(fs, byteorder); // 1
        std::uint8_t sig2 = readValue<std::uint8_t>(fs, byteorder); // 187
        std::uint8_t sig3 = readValue<std::uint8_t>(fs, byteorder); // 118
        std::uint8_t sig4 = readValue<std::uint8_t>(fs, byteorder); // 152
        std::uint8_t sig5 = readValue<std::uint8_t>(fs, byteorder); // 255
        if (sig1 != 1 || sig2 != 187 || sig3 != 118 || sig4 != 152 || sig5 != 255) {
          Rcpp::warning("Unexpected value %i %i %i %i %i", sig1, sig2, sig3, sig4, sig5);
        }
        
        std::uint32_t value = readValue<std::uint32_t>(fs, byteorder); // 1
        if (value != 1) {
          Rcpp::warning("Unexpected value %i", value);
        }
        
        std::uint64_t ncat = readValue<std::uint64_t>(fs, byteorder);
        uint8_t idxbytes = readValue<std::uint8_t>(fs, byteorder);
        switch (idxbytes) {
        case 1:
          result = readCategoricalColumn<std::uint8_t>(fs, nrec, ncat, byteorder, enc);
          break;
        case 2:
          result = readCategoricalColumn<std::uint16_t>(fs, nrec, ncat, byteorder, enc);
          break;
        case 4:
          result = readCategoricalColumn<std::uint32_t>(fs, nrec, ncat, byteorder, enc);
          break;
        default:
          Rcpp::warning("Unhandled size: %i", idxbytes);
        }
        auto colend = fs.tellg();
        std::uint64_t sizeread = colend - colstart;
        if (collenbytes != sizeread) {
          Rcpp::warning("Unexpected number of bytes read expected: %i got: %i", collenbytes, sizeread);
        }
      }
    }
    else {
      if (version < 7) {
        result = readFixedLengthStringColumn<std::uint8_t>(fs, nrec, byteorder, recwidth1 - 1, enc);
      }
      else {
        result = readCStringColumn<std::uint8_t>(fs, nrec, recwidth1, enc);
      }
    }
  }
  else if (datatype == 3) { // RowState
    if (version < 7) {
      switch (recwidth2) {
      case 2:
        result = readRowStateColumn<std::uint16_t>(fs, nrec, byteorder);
        break;
      default:
        Rcpp::warning("Unhandled rowstate size: %i", recwidth2);
      }
    }
    else {
      switch (recwidth1) {
      case 8:
        result = readRowStateColumn<std::uint64_t>(fs, nrec, byteorder);
        break;
      default:
        Rcpp::warning("Unhandled rowstate size %i", recwidth1);
      }
    }
  }
  else if (datatype == 255) {
    if (version < 7) { // Integer (always little-endian)
      switch (recwidth2) {
      case 1: // byte
        result = readIntegerColumn<std::int8_t>(fs, nrec, std::endian::little);
        break;
      case 2: // short
        result = readIntegerColumn<std::int16_t>(fs, nrec, std::endian::little);
        break;
      case 4: // int
        result = readIntegerColumn<std::int32_t>(fs, nrec, std::endian::little);
        break;
      default:
        Rcpp::warning("Unhandled size: %i", recwidth2);
      }
    }
    else { // byte (-1)
      result = readIntegerColumn<std::int8_t>(fs, nrec, byteorder);
    }
  }
  else if (datatype == 254) {
    if (version < 7) { // Integer (byteorder dependent on file definition)
      switch (recwidth2) {
      case 1: // byte
        result = readIntegerColumn<std::int8_t>(fs, nrec, byteorder);
        break;
      case 2: // short
        result = readIntegerColumn<std::int16_t>(fs, nrec, byteorder);
        break;
      case 4: // int
        result = readIntegerColumn<std::int32_t>(fs, nrec, byteorder);
        break;
      default:
        Rcpp::warning("Unhandled size: %i", recwidth2);
      }
    }
    else { // short (-2)
      result = readIntegerColumn<std::int16_t>(fs, nrec, byteorder);
    }
  }
  else if (datatype == 252) { // Int (-4)
    result = readIntegerColumn<std::int32_t>(fs, nrec, byteorder);
  }
  return result;
}

[[nodiscard]] static Rcpp::DataFrame readTable(std::uint32_t version, std::istream& fs, std::endian byteorder, std::uint64_t nrec, std::uint32_t nvar) {
  Rcpp::DataFrame result;
  encoding enc = encoding::Native;
  
  // Table-level attributes
  std::uint16_t tabattrcount = readValue<std::uint16_t>(fs, byteorder); // Number of records
  std::uint32_t tabattrblocksize = 0;
  if (tabattrcount != 0) {
    tabattrblocksize = readValue<std::uint32_t>(fs, byteorder); // Total bytes
  }
  auto tabattrstart = fs.tellg();
  std::string cset = "";
  for (std::uint16_t i = 0; i < tabattrcount; ++i) {
    std::uint16_t rtype = readValue<std::uint16_t>(fs, byteorder);
    std::uint32_t rsize = readValue<std::uint32_t>(fs, byteorder);
    if (rtype == 1) {
      std::string label = Enc2UTF8(readString(fs, rsize), enc); // Label
      result.attr("label") = label;
    }
    else if (rtype == 6) { // Character set
      cset = Enc2UTF8(readCString(fs, rsize), enc);
      if (cset == "utf-8") {
        enc = encoding::UTF8;
      }
      else if (cset == "x-mac-roman") {
        enc = encoding::XMacRoman;
      }
      else {
        enc = encoding::Unknown;
        Rcpp::warning("Unknown character encoding: %s", cset.c_str());
      }
    }
    else if (rtype == 7) { // Creation date
      double dt = readValue<double>(fs, byteorder); // save time
      std::chrono::sys_time tp{ std::chrono::seconds{ static_cast<std::int64_t>(dt - 2082844800) } }; // Convert from 1st January 1904 to 1st January 1970 epoch
      Rcpp::Rcout << std::format("Creation date: {}", tp) << std::endl;
    }
    else if (rtype == 18) {
      std::string ver = Enc2UTF8(readString(fs, rsize), enc); // release information
      Rcpp::Rcout << ver << std::endl;
    }
    else {
      if (rsize > 0) {
        skipBytes(fs, rsize);
      }
    }
  }
  auto tabattrend = fs.tellg();
  std::uint64_t tabattrread = tabattrend - tabattrstart;
  if (tabattrblocksize != 0 && tabattrread != tabattrblocksize) {
    Rcpp::warning("Table attributes 1: Unexpected number of bytes read expected: %i got: %i", tabattrblocksize, tabattrread);
  }
  
  if (version >= 7) {
    std::uint16_t tabinforecordcount = readValue<std::uint16_t>(fs, byteorder); // number of records
    std::uint64_t tabinfoblocksize = readValue<std::uint64_t>(fs, byteorder); // size in bytes
    auto tabinfostart = fs.tellg();
    for (uint16_t i = 0; i < tabinforecordcount; ++i) {
      [[maybe_unused]] std::uint16_t itype = readValue<std::uint16_t>(fs, byteorder);
      std::uint64_t isize = readValue<std::uint64_t>(fs, byteorder);
      skipBytes(fs, isize);
    }
    auto tabinfoend = fs.tellg();
    std::uint64_t tabinforead = tabinfoend - tabinfostart;
    if (tabinfoblocksize != 0 && tabinforead != tabinfoblocksize) {
      Rcpp::warning("Table attributes 2: Unexpected number of bytes read expected: %i got: %i", tabinfoblocksize, tabinforead);
    }
  }
  
  // Column information
  std::vector<std::streampos> colstarts(nvar);
  
  if (version < 7) {
    std::uint16_t colstartssize = readValue<std::uint16_t>(fs, byteorder);
    if (colstartssize != 0) {
      std::uint16_t bytesread = 0;
      for (std::uint32_t i = 0; i < nvar; ++i) {
        colstarts[i] = readValue<std::uint32_t>(fs, byteorder);
        bytesread += 4;
      }
      if (bytesread != colstartssize) {
        std::vector<std::uint8_t> extra(colstartssize - bytesread);
        fs.read((char*)extra.data(), extra.size());
      }
    }
  }
  else {
    std::uint32_t numstarts = readValue<std::uint32_t>(fs, byteorder);
    for (std::uint64_t i = 0; i < numstarts; ++i) {
      colstarts[i] = readValue<std::uint64_t>(fs, byteorder);
    }
  }
  bool extprecision = false;
  for (std::uint32_t i = 0; i < colstarts.size(); ++i) {
    auto pos = fs.tellg();
    if (colstarts[i] != 0 && colstarts[i] != pos) {
      Rcpp::warning("Unexpected column start position: %i, should be %i", std::size_t(pos), std::size_t(colstarts[i]));
      // Go to start of next column
      fs.clear();
      fs.seekg(colstarts[i], std::ios::beg);
    }
    
    std::uint16_t namelen = (version < 7) ? readValue<std::uint8_t>(fs, byteorder): readValue<std::uint16_t>(fs, byteorder);
    std::string name = (version < 7) ? readString(fs, 31) : readString(fs, namelen);
    name.resize(namelen);
    name = Enc2UTF8(name, enc);

    std::uint8_t datatype = readValue<std::uint8_t>(fs, byteorder); // data type (1: numeric 2: string 3: rowstate 9: string(compressed) 10: numeric(compressed))
    [[maybe_unused]] std::uint8_t meastype = readValue<std::uint8_t>(fs, byteorder); // measure (0: continuous 1: ordinal 2: nominal 3: rowstate)
    [[maybe_unused]] std::uint8_t fwidth = readValue<std::uint8_t>(fs, byteorder); // Field width
    std::uint8_t fdata = readValue<std::uint8_t>(fs, byteorder); // can be 0, same as fwidth or record width 1, special values > 101 for date/time
    // In version <= 3 recwidth seems to have been a single 16-bit integer and therefore the values switch around due to byte order
    // For fixed strings and numeric types contains number of bytes for each record, is zero for variable strings
    std::uint8_t recwidth1 = readValue<std::uint8_t>(fs, byteorder);
    std::uint8_t recwidth2 = readValue<std::uint8_t>(fs, byteorder);
    [[maybe_unused]] std::uint16_t editstate = readValue<std::uint16_t>(fs, byteorder); // 0: Unset 1: Locked 2: Formula
    if (version >= 7) {
      if (datatype == 1 || datatype == 10) { // Floating-point numeric
        [[maybe_unused]] std::uint8_t cval1 = readValue<std::uint8_t>(fs, byteorder);
      }
      [[maybe_unused]] std::uint16_t cval2 = readValue<std::uint16_t>(fs, byteorder);
    }
    
    std::uint16_t colattrcount = readValue<std::uint16_t>(fs, byteorder);
    std::uint64_t colattrblocksize = 0;
    if (version >= 7) {
      colattrblocksize = readValue<std::uint64_t>(fs, byteorder);
    }
    if (colattrcount > 0) {
      if (version < 7) {
        colattrblocksize = readValue<std::uint32_t>(fs, byteorder);
      }
      auto colattrstart = fs.tellg();
      std::uint64_t totalcontentsize = 0;
      for (std::uint16_t j = 0; j < colattrcount; ++j) {
        std::uint16_t recordtype = readValue<std::uint16_t>(fs, byteorder);
        std::uint64_t recordsize = (version < 7) ? readValue<std::uint32_t>(fs, byteorder) : readValue<std::uint64_t>(fs, byteorder);
        if (recordtype == 1) { // Label
          result.attr("label") = Enc2UTF8(readString(fs, recordsize), enc);
        }
        else {
          skipBytes(fs, recordsize);
        }
        totalcontentsize += recordsize;
      }
      auto colattrend = fs.tellg();
      // colattrblocksize can have one of three values:
      // When reading version >= 7 (little-endian) sizeread == colattrblocksize, including in version 3 warning table at start.
      // When reading version == 3 (bug-endian) (sizeread + 6) == colattrblocksize. The +6 is to include colattrcount and colattrblocksize
      // When reading version == 3 (big-endian) totalcontentsize == colattrblocksize. This only includes the sum of record content
      std::uint64_t sizeread = colattrend - colattrstart;
      if (colattrblocksize != 0 && sizeread != colattrblocksize && (sizeread + 6) != colattrblocksize && totalcontentsize != colattrblocksize) {
        Rcpp::warning("Column attributes 1: Unexpected number of column attribute bytes read expected: %i got: %i", colattrblocksize, sizeread);
      }
      if (colattrblocksize == 0) { // Not sure if condition for reading this is correct (only encountered so far in extra warning table for new versions)
        [[maybe_unused]] std::uint32_t extravalue = readValue<std::uint32_t>(fs, byteorder); // (0)
      }
    }
    
    if (version >= 7) {
      std::uint16_t colinfocount = readValue<std::uint16_t>(fs, byteorder);
      std::uint32_t colinfoblocksize = readValue<std::uint32_t>(fs, byteorder);
      auto colinfostart = fs.tellg();
      for (std::uint16_t j = 0; j < colinfocount; ++j) {
        [[maybe_unused]] std::uint16_t fieldtype = readValue<std::uint16_t>(fs, byteorder);
        std::uint32_t fieldsize = readValue<std::uint32_t>(fs, byteorder);
        skipBytes(fs, fieldsize);
      }
      auto colinfoend = fs.tellg();
      std::uint64_t colinforead = colinfoend - colinfostart;
      if (colinfoblocksize != 0 && colinforead != colinfoblocksize) {
        Rcpp::warning("Column attributes 2: Unexpected number of bytes read expected: %i got: %i", colinfoblocksize, colinforead);
      }
    }
    
    if (datatype == 9 || datatype == 10) {
      datatype = readValue<std::uint8_t>(fs, byteorder); // Actual data type
      std::uint8_t sig1 = readValue<std::uint8_t>(fs, byteorder);
      std::uint8_t sig2 = readValue<std::uint8_t>(fs, byteorder);
      std::uint8_t sig3 = readValue<std::uint8_t>(fs, byteorder);
      std::uint8_t sig4 = readValue<std::uint8_t>(fs, byteorder);
      std::uint8_t sig5 = readValue<std::uint8_t>(fs, byteorder);
      std::uint8_t sig6 = readValue<std::uint8_t>(fs, byteorder);
      std::uint8_t sig7 = readValue<std::uint8_t>(fs, byteorder);
      if (sig1 != 1 || sig2 != 0 || sig3 != 0 || sig4 != 239 || sig5 != 190 || sig6 != 254 || sig7 != 202) {
        Rcpp::warning("Unexpected value %i %i %i %i %i %i %i", sig1, sig2, sig3, sig4, sig5, sig6, sig7);
      }
      
      std::uint64_t compsz = readValue<std::uint64_t>(fs, byteorder);
      std::uint64_t uncompsz = readValue<std::uint64_t>(fs, byteorder);

      z_stream strm = {};
      constexpr auto CHUNK = 16384;
      std::vector<char> in(CHUNK, '\0');
      std::vector<char> out(CHUNK, '\0');
      
      /* allocate inflate state */
      strm.zalloc = Z_NULL;
      strm.zfree = Z_NULL;
      strm.opaque = Z_NULL;
      strm.avail_in = 0;
      strm.next_in = Z_NULL;

      std::uint64_t bytesread = 0;
      std::stringstream outstr(std::ios::in | std::ios::out | std::ios::binary);
      int ret = inflateInit2(&strm, 16 + MAX_WBITS);
      if (ret == Z_OK) {
        /* decompress until deflate stream ends or end of file */
        std::uint64_t bytesleft = compsz;
        do {
          auto readsize = std::min(std::size_t(CHUNK), bytesleft);
          fs.read((char*)in.data(), readsize);
          bytesleft -= readsize;
          strm.avail_in = (uInt)readsize;
          if (strm.avail_in == 0)
            break;
          strm.next_in = (Bytef*)in.data();
          
          do {
            strm.avail_out = CHUNK;
            strm.next_out = (Bytef*)out.data();
            ret = inflate(&strm, Z_NO_FLUSH);
            switch (ret) {
            case Z_STREAM_ERROR:
              [[fallthrough]];
            case Z_NEED_DICT:
              [[fallthrough]];
            case Z_DATA_ERROR:
              [[fallthrough]];
            case Z_MEM_ERROR:
              strm.avail_out = 0;
              break;
            }
            unsigned have = CHUNK - strm.avail_out;
            outstr.write(out.data(), have);
            bytesread += have;
          } while (strm.avail_out == 0);
        } while (ret != Z_STREAM_END);
        inflateEnd(&strm);
      }
      
      outstr.flush();
      outstr.clear();
      outstr.seekg(0);
      
      if (bytesread != uncompsz) {
        Rcpp::warning("Unexpected number of bytes decompressed: expected: %i got: %i", uncompsz, bytesread);
      }
      
      result.push_back(readColumn(version, outstr, byteorder, enc, nrec, datatype, fdata, recwidth1, recwidth2), name);

      std::uint64_t sizeread = outstr.tellg();
      if (sizeread != uncompsz) {
        Rcpp::warning("Incorrect number of bytes read: expected %i got %i", uncompsz, sizeread);
      }
    }
    else {
      result.push_back(readColumn(version, fs, byteorder, enc, nrec, datatype, fdata, recwidth1, recwidth2), name);
    }
    if (datatype == 1 && recwidth2 == 10) {
      extprecision = true;
    }
  }

  if (extprecision == true) {
    Rcpp::warning("Extended-precision floating point numbers have been rounded to double");
  }
  return result;
}

//' rcpp_readjmp
//' 
//' This function allows reading data the JMP data format
//' 
//' @param filename name of file to open
//' @return object of type "data.frame"
//' @export
// [[Rcpp::export]]
Rcpp::DataFrame rcpp_readjmp(std::string filename) {
  Rcpp::DataFrame result;
  std::fstream fs(filename, std::ios::in | std::ifstream::binary);
  if (!fs.good()) {
    Rcpp::stop("Unable to open file %s\n", filename);
  }
  std::vector<std::uint8_t> sig(4);
  fs.read((char*)sig.data(), sig.size());
  std::endian byteorder = std::endian::native;
  if (sig[0] == 0 && sig[1] == 0 && sig[2] == 0xff && sig[3] == 0xff) {
    byteorder = std::endian::big;
  }
  else if (sig[0] == 0xff && sig[1] == 0xff && sig[2] == 0 && sig[3] == 0) {
    byteorder = std::endian::little;
  }
  else {
    Rcpp::stop("Unrecognised file format");
  }
  std::uint32_t ver = readValue<std::uint32_t>(fs, byteorder);
  std::uint64_t nrec = readValue<std::uint32_t>(fs, byteorder);
  std::uint32_t nvar = readValue<std::uint16_t>(fs, byteorder);
  std::uint16_t newerversion = readValue<std::uint16_t>(fs, byteorder);
  result = readTable(std::min(ver, std::uint32_t(3)), fs, byteorder, nrec, nvar);
  if (newerversion > 0) {
    ver = readValue<std::uint32_t>(fs, byteorder);
    nrec = readValue<std::uint64_t>(fs, byteorder);
    nvar = readValue<std::uint32_t>(fs, byteorder);
    newerversion = readValue<std::uint16_t>(fs, byteorder);
    result = readTable(std::min(ver, std::uint32_t(7)), fs, byteorder, nrec, nvar);
    if (newerversion != 0) { // Newer version
      ver = readValue<std::uint32_t>(fs, byteorder);
      nrec = readValue<std::uint64_t>(fs, byteorder);
      nvar = readValue<std::uint32_t>(fs, byteorder);
      newerversion = readValue<std::uint16_t>(fs, byteorder);
      result = readTable(std::min(ver, std::uint32_t(8)), fs, byteorder, nrec, nvar);
      if (newerversion != 0) { // Newer version
        Rcpp::warning("Unrecognised file format %i", ver);
      }
    }
  }
  Rcpp::Rcout << "JMP file format version: " << ver << std::endl;
  
  return result;
}
