#include "yet_rtmp_amf_op.h"
#include <string.h>
#include <stdlib.h>
#include <sstream>

/// NOTICE all use 2 bytes name len
#define ENCODE_OBJECT_NAME(out, name, name_len) \
  out = encode_int16(out, name_len); \
  memcpy(out, name, name_len); \
  out += name_len;

namespace yet {

uint8_t *AmfOp::encode_int16(uint8_t *out, int16_t val) {
  *out++ = val >> 8;
  *out++ = val & 0xff;
  return out;
}

uint8_t *AmfOp::encode_int24(uint8_t *out, int32_t val) {
  *out++ = val >> 16;
  *out++ = val >> 8;
  *out++ = val & 0xff;
  return out;
}

uint8_t *AmfOp::encode_int32(uint8_t *out, int32_t val) {
  *out++ = val >> 24;
  *out++ = val >> 16;
  *out++ = val >> 8;
  *out++ = val & 0xff;
  return out;
}

uint8_t *AmfOp::encode_int32_le(uint8_t *out, int32_t val) {
  *out++ = val & 0xff;
  *out++ = val >> 8;
  *out++ = val >> 16;
  *out++ = val >> 24;
  return out;
}


uint8_t *AmfOp::encode_number(uint8_t *out, double val) {
  *out++ = Amf0DataType_NUMBER;
  uint8_t *p = (uint8_t *)&val;
  out[0] = p[7];
  out[1] = p[6];
  out[2] = p[5];
  out[3] = p[4];
  out[4] = p[3];
  out[5] = p[2];
  out[6] = p[1];
  out[7] = p[0];
  return (out+8);
}

uint8_t *AmfOp::encode_boolean(uint8_t *out, bool val) {
  *out++ = Amf0DataType_BOOLEAN;
  *out++ = val ? 0x01 : 0x00;
  return out;
}

uint8_t *AmfOp::encode_string(uint8_t *out, const char *val, int val_len) {
  if (val_len < 65536) {
    *out++ = Amf0DataType_STRING;
    out = encode_int16(out, val_len);
  } else {
    *out++ = Amf0DataType_LONG_STRING;
    out = encode_int32(out, val_len);
  }
  if (val_len > 0) { memcpy(out, val, val_len); }

  return (out+val_len);
}

uint8_t *AmfOp::encode_null(uint8_t *out) {
  *out++ = Amf0DataType_NULL;
  return out;
}

uint8_t *AmfOp::encode_object_begin(uint8_t *out) {
  *out++ = Amf0DataType_OBJECT;
  return out;
}

uint8_t *AmfOp::encode_object_named_boolean(uint8_t *out, const char *name, int name_len, bool val) {
  ENCODE_OBJECT_NAME(out, name, name_len);
  return encode_boolean(out, val);
}

uint8_t *AmfOp::encode_object_named_number(uint8_t *out, const char *name, int name_len, double val) {
  ENCODE_OBJECT_NAME(out, name, name_len);
  return encode_number(out, val);
}

uint8_t *AmfOp::encode_object_named_string(uint8_t *out, const char *name, int name_len, const char *val, int val_len) {
  ENCODE_OBJECT_NAME(out, name, name_len);
  return encode_string(out, val, val_len);
}

uint8_t *AmfOp::encode_object_end(uint8_t *out) {
  return encode_int24(out, Amf0DataType_OBJECT_END);
}

uint8_t *AmfOp::encode_ecma_array_begin(uint8_t *out, uint32_t array_len) {
  *out++ = Amf0DataType_ECMA_ARRAY;
  return encode_int32(out, array_len);
}

uint8_t *AmfOp::encode_ecma_array_end(uint8_t *out) {
  return encode_int24(out, Amf0DataType_OBJECT_END);
}

uint8_t *AmfOp::decode_boolean_with_type(const uint8_t *in, std::size_t valid_len, bool *out, std::size_t *used_len) {
  if (!in || valid_len < 2 || !out || *in != Amf0DataType_BOOLEAN) { return NULL; }

  *out = (*(in+1) == 0x01);
  if (used_len) { *used_len = 2; }

  return (uint8_t *)in+2;
}

uint8_t *AmfOp::decode_number_with_type(const uint8_t *in, std::size_t valid_len, double *out, std::size_t *used_len) {
  if (!in || valid_len < 9 || !out || (*in != Amf0DataType_NUMBER)) { return NULL; }

  uint8_t *p = (uint8_t *)out;
  p[0] = in[8];
  p[1] = in[7];
  p[2] = in[6];
  p[3] = in[5];
  p[4] = in[4];
  p[5] = in[3];
  p[6] = in[2];
  p[7] = in[1];
  if (used_len) { *used_len = 9; }

  return (uint8_t *)in+9;
}

uint8_t *AmfOp::decode_int16(const uint8_t *in, std::size_t valid_len, int32_t *out, std::size_t *used_len) {
  if (!in || valid_len < 2 || !out) { return NULL; }

  uint8_t *p = (uint8_t *)out;
  *p++ = in[1];
  *p++ = in[0];
  *p++ = 0;
  *p = 0;
  if (used_len) { *used_len = 2; }

  return (uint8_t *)in+2;
}

uint8_t *AmfOp::decode_int24(const uint8_t *in, std::size_t valid_len, int32_t *out, std::size_t *used_len) {
  if (!in || valid_len < 3 || !out) { return NULL; }

  uint8_t *p = (uint8_t *)out;
  *p++ = in[2];
  *p++ = in[1];
  *p++ = in[0];
  *p = 0;
  if (used_len) { *used_len = 3; }

  return (uint8_t *)in+3;
}

uint8_t *AmfOp::decode_int32(const uint8_t *in, std::size_t valid_len, int32_t *out, std::size_t *used_len) {
  if (!in || valid_len < 4 || !out) { return NULL; }

  uint8_t *p = (uint8_t *)out;
  *p++ = in[3];
  *p++ = in[2];
  *p++ = in[1];
  *p   = in[0];
  if (used_len) { *used_len = 4; }

  return (uint8_t *)in+4;
}

uint8_t *AmfOp::decode_int32_le(const uint8_t *in, std::size_t valid_len, int32_t *out, std::size_t *used_len) {
  if (!in || valid_len < 4 || !out) { return NULL; }

  uint8_t *p = (uint8_t *)out;
  *p++ = in[0];
  *p++ = in[1];
  *p++ = in[2];
  *p   = in[3];
  if (used_len) { *used_len = 4; }

  return (uint8_t *)in+4;
}

uint8_t *AmfOp::decode_string(const uint8_t *in, std::size_t valid_len, char **out, int *str_len, std::size_t *used_len) {
  if (!in || valid_len < 2 || !str_len) { return NULL; }

  *str_len = (in[0] << 8) + in[1];
  if (*str_len > int(valid_len-2)) { return NULL; }

  *out = (char *)in + 2;
  if (used_len) { *used_len = *str_len + 2; }

  return (uint8_t *)in + (*str_len) + 2;
}

uint8_t *AmfOp::decode_string_with_type(const uint8_t *in, std::size_t valid_len, char **out, int *str_len, std::size_t *used_len) {
  if (!in || valid_len < 3 || !str_len) { return NULL; }

  if (in[0] != Amf0DataType_STRING) { return NULL; }

  uint8_t *ret = decode_string(in+1, valid_len-1, out, str_len, used_len);
  if (ret == NULL) { return NULL; }

  if (used_len) { (*used_len)++; }

  return ret;
}

static uint8_t *decode_object_or_ecma_array(const uint8_t *in, std::size_t valid_len, AmfObjectItemMap *objs, std::size_t *used_len) {
  uint8_t *p = (uint8_t *)in;
  uint8_t *q;
  int str_len;
  double number;
  bool b;
  std::string k;
  std::size_t ul;

  for (; ; ) {

    // object-end-marker 0x0 0x0 0x9
    if (valid_len >= 3 && p[0] == 0 && p[1] == 0 && p[2] == 9) {
      ul = p + 3 - in;

      if (used_len) { *used_len = ul; }
      return (p+3);
    }

    p = AmfOp::decode_string(p, valid_len, (char **)&q, &str_len, &ul);
    if (!p) { goto failed; }

    k.assign((const char *)q, str_len);
    valid_len -= ul;

    if (*p == Amf0ObjectValueType_NUMBER) {
      p = AmfOp::decode_number_with_type(p, valid_len, &number, &ul);
      if (!p) { goto failed; }

      objs->put(k, number);
      valid_len -= ul;
    } else if (*p == Amf0ObjectValueType_BOOLEAN) {
      p = AmfOp::decode_boolean_with_type(p, valid_len, &b, &ul);
      if (!p) { goto failed; }

      valid_len -= ul;
      objs->put(k, b);
    } else if (*p == Amf0ObjectValueType_STRING) {
      p = AmfOp::decode_string_with_type(p, valid_len, (char **)&q, &str_len, &ul);
      if (!p) { goto failed; }

      objs->put(k, (const char *)q, str_len);
      valid_len = valid_len - ul;
    } else {
      return nullptr;
    }
  }

failed:
  return nullptr;

}

uint8_t *AmfOp::decode_object(const uint8_t *in, std::size_t valid_len, AmfObjectItemMap *objs, std::size_t *used_len) {
  if (!in || !objs) { return nullptr; }
  if (*in != Amf0DataType_OBJECT) { return nullptr; }

  return decode_object_or_ecma_array(in+1, valid_len-1, objs, used_len);
}

uint8_t *AmfOp::decode_ecma_array(const uint8_t *in, std::size_t valid_len, AmfObjectItemMap *objs, std::size_t *used_len) {
  if (!in || !objs) { return nullptr; }
  if (*in != Amf0DataType_ECMA_ARRAY) { return nullptr; }

  int32_t array_len;
  uint8_t *p = decode_int32(in+1, valid_len-1, &array_len, nullptr);
  if (!p) { return nullptr; }

  // array_len TODO obs 14 means 20(1*16+4)
  (void)array_len;
  return decode_object_or_ecma_array(p, valid_len-5, objs, used_len);
  //return (!p && objs->size() == array_len) ? p : nullptr;
}

AmfObjectItemMap::AmfObjectItemMap() {}

AmfObjectItemMap::~AmfObjectItemMap() { clear(); }

#define SNIPPET_AOIM_PUT(name, v, t) \
  auto not_exist = true; \
  auto iter = objs_.find(name); \
  if (iter != objs_.end()) { \
    list_.erase(iter->second); \
    objs_.erase(iter); \
    not_exist = false; \
  } \
  auto iter2 = list_.insert(list_.end(), std::make_pair(name, new t(v))); \
  objs_[name] = iter2; \
  return not_exist;

bool AmfObjectItemMap::put(const std::string &name, bool v) {
  SNIPPET_AOIM_PUT(name, v, AmfObjectItemBoolean);
}

bool AmfObjectItemMap::put(const std::string &name, double v) {
  SNIPPET_AOIM_PUT(name, v, AmfObjectItemNumber);
}

bool AmfObjectItemMap::put(const std::string &name, const std::string &v) {
  SNIPPET_AOIM_PUT(name, v, AmfObjectItemString);
}

bool AmfObjectItemMap::put(const std::string &name, const char *v, int len) {
  return put(name, std::string(v, len));
}

AmfObjectItem *AmfObjectItemMap::get(const std::string &name) {
  auto iter = objs_.find(name);
  if (iter == objs_.end()) { return nullptr; }

  return iter->second->second;
}

std::string AmfObjectItemMap::stringify() {
  std::ostringstream oss;
  for (auto &iter : list_) {
    oss << "{<" << iter.first << ">:"
        << "<" << iter.second->stringify() << ">}";
  }
  return oss.str();
}

void AmfObjectItemMap::clear() {
  for (auto &iter : list_) { delete iter.second; }

  objs_.clear();
  list_.clear();
}

} // namespace yet
