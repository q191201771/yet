#include "rtmp_amf_op.h"
#include <string.h>
#include <stdlib.h>
#include <sstream>

/// 注意,object中的name统一用2字节len头
#define ENCODE_OBJECT_NAME(out, name, name_len) \
  out = encode_int16(out, name_len); \
  memcpy(out, name, name_len); \
  out += name_len;

namespace yet {

char *AmfOp::encode_int16(char *out, int16_t val) {
  *out++ = val >> 8;
  *out++ = val & 0xff;
  return out;
}

char *AmfOp::encode_int24(char *out, int32_t val) {
  *out++ = val >> 16;
  *out++ = val >> 8;
  *out++ = val & 0xff;
  return out;
}

char *AmfOp::encode_int32(char *out, int32_t val) {
  *out++ = val >> 24;
  *out++ = val >> 16;
  *out++ = val >> 8;
  *out++ = val & 0xff;
  return out;
}

char *AmfOp::encode_int32_le(char *out, int32_t val) {
  *out++ = val & 0xff;
  *out++ = val >> 8;
  *out++ = val >> 16;
  *out++ = val >> 24;
  return out;
}


char *AmfOp::encode_number(char *out, double val) {
  *out++ = Amf0DataType_NUMBER;
  char *p = (char *)&val;
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

char *AmfOp::encode_boolean(char *out, bool val) {
  *out++ = Amf0DataType_BOOLEAN;
  *out++ = val ? 0x01 : 0x00;
  return out;
}

char *AmfOp::encode_string(char *out, const char *val, int val_len) {
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

char *AmfOp::encode_null(char *out) {
  *out++ = Amf0DataType_NULL;
  return out;
}

char *AmfOp::encode_object_begin(char *out) {
  *out++ = Amf0DataType_OBJECT;
  return out;
}

char *AmfOp::encode_object_named_boolean(char *out, const char *name, int name_len, bool val) {
  ENCODE_OBJECT_NAME(out, name, name_len);
  return encode_boolean(out, val);
}

char *AmfOp::encode_object_named_number(char *out, const char *name, int name_len, double val) {
  ENCODE_OBJECT_NAME(out, name, name_len);
  return encode_number(out, val);
}

char *AmfOp::encode_object_named_string(char *out, const char *name, int name_len, const char *val, int val_len) {
  ENCODE_OBJECT_NAME(out, name, name_len);
  return encode_string(out, val, val_len);
}

char *AmfOp::encode_object_end(char *out) {
  return encode_int24(out, Amf0DataType_OBJECT_END);
}

char *AmfOp::encode_ecma_array_begin(char *out, int array_len) {
  *out++ = Amf0DataType_ECMA_ARRAY;
  return encode_int32(out, array_len);
}

char *AmfOp::encode_ecma_array_end(char *out) {
  return encode_int24(out, Amf0DataType_OBJECT_END);
}

char *AmfOp::decode_boolean_with_type(const char *in, int valid_len, bool *out, int *used_len) {
  if (!in || valid_len < 2 || !out || *in != Amf0DataType_BOOLEAN) { return NULL; }

  *out = (*(in+1) == 0x01);
  if (used_len) { *used_len = 2; }

  return (char *)in+2;
}

char *AmfOp::decode_int16(const char *in, int valid_len, int32_t *out, int *used_len) {
  if (!in || valid_len < 2 || !out) { return NULL; }

  char *p = (char *)out;
  *p++ = in[1];
  *p++ = in[0];
  *p++ = 0;
  *p = 0;
  if (used_len) { *used_len = 2; }

  return (char *)in+2;
}

char *AmfOp::decode_int24(const char *in, int valid_len, int32_t *out, int *used_len) {
  if (!in || valid_len < 3 || !out) { return NULL; }

  char *p = (char *)out;
  *p++ = in[2];
  *p++ = in[1];
  *p++ = in[0];
  *p = 0;
  if (used_len) { *used_len = 3; }

  return (char *)in+3;
}

char *AmfOp::decode_int32(const char *in, int valid_len, int32_t *out, int *used_len) {
  if (!in || valid_len < 4 || !out) { return NULL; }

  char *p = (char *)out;
  *p++ = in[3];
  *p++ = in[2];
  *p++ = in[1];
  *p   = in[0];
  if (used_len) { *used_len = 4; }

  return (char *)in+4;
}

char *AmfOp::decode_int32_le(const char *in, int valid_len, int32_t *out, int *used_len) {
  if (!in || valid_len < 4 || !out) { return NULL; }

  char *p = (char *)out;
  *p++ = in[0];
  *p++ = in[1];
  *p++ = in[2];
  *p   = in[3];
  if (used_len) { *used_len = 4; }

  return (char *)in+4;
}


char *AmfOp::decode_number_with_type(const char *in, int valid_len, double *out, int *used_len) {
  if (!in || valid_len < 9 || !out || (*in != Amf0DataType_NUMBER)) { return NULL; }

  char *p = (char *)out;
  p[0] = in[8];
  p[1] = in[7];
  p[2] = in[6];
  p[3] = in[5];
  p[4] = in[4];
  p[5] = in[3];
  p[6] = in[2];
  p[7] = in[1];
  if (used_len) { *used_len = 9; }

  return (char *)in+9;
}

char *AmfOp::decode_string(const char *in, int valid_len, char **out, int *str_len, int *used_len) {
  if (!in || valid_len < 2 || !str_len) { return NULL; }

  *str_len = (in[0] << 8) + in[1];
  if (*str_len > (valid_len-2)) { return NULL; }

  *out = (char *)in + 2;
  if (used_len) { *used_len = *str_len + 2; }

  return (char *)in + (*str_len) + 2;
}

char *AmfOp::decode_string_with_type(const char *in, int valid_len, char **out, int *str_len, int *used_len) {
  if (!in || valid_len < 3 || !str_len) { return NULL; }

  if (in[0] != Amf0DataType_STRING) { return NULL; }

  char *ret = decode_string(in+1, valid_len-1, out, str_len, used_len);
  if (ret == NULL) { return NULL; }

  if (used_len) { (*used_len)++; }

  return ret;
}

char *AmfOp::decode_object(const char *in, int valid_len, AmfObjectItemMap *objs, int *used_len) {
  if (!in) { return NULL; }

  char *p = (char *)in;
  char *q;
  int str_len;
  double number;
  bool b;
  std::string k;
  int ul;
  if (*p++ != Amf0DataType_OBJECT) { goto failed; }

  valid_len--;

  for (; ; ) {

    if (valid_len >= 3 && p[0] == 0 && p[1] == 0 && p[2] == 9) {
      ul = p + 3 - in;

      if (used_len) { *used_len = ul; }
      return (p+3);
    }

    p = decode_string(p, valid_len, &q, &str_len, &ul);
    if (!p) { goto failed; }

    k.assign(q, str_len);
    valid_len -= ul;

    if (*p == Amf0ObjectValueType_NUMBER) {
      p = decode_number_with_type(p, valid_len, &number, &ul);
      if (!p) { goto failed; }

      objs->put(k, number);
      valid_len -= ul;
    } else if (*p == Amf0ObjectValueType_BOOLEAN) {
      p = decode_boolean_with_type(p, valid_len, &b, &ul);
      if (!p) { goto failed; }

      valid_len -= ul;
      objs->put(k, b);
    } else if (*p == Amf0ObjectValueType_STRING) {
      //p++;
      //valid_len--;
      p = decode_string_with_type(p, valid_len, &q, &str_len, &ul);
      if (!p) { goto failed; }

      objs->put(k, q, str_len);
      valid_len = valid_len - ul;
    }
  }

  assert(0);
  return NULL;

failed:
  return NULL;
}

AmfObjectItemMap::AmfObjectItemMap() {}

AmfObjectItemMap::~AmfObjectItemMap() {
  clear();
}

bool AmfObjectItemMap::put(const std::string &name, bool v) {
  bool not_exist = true;
  std::map<std::string, AmfObjectItem *>::iterator iter = objs_.find(name);
  if (iter != objs_.end()) {
    objs_.erase(iter);
    not_exist = false;
  }
  objs_.insert(std::make_pair(name, new AmfObjectItemBoolean(v)));
  return not_exist;
}

bool AmfObjectItemMap::put(const std::string &name, double v) {
  bool not_exist = true;
  std::map<std::string, AmfObjectItem *>::iterator iter = objs_.find(name);
  if (iter != objs_.end()) {
    objs_.erase(iter);
    not_exist = false;
  }
  objs_.insert(std::make_pair(name, new AmfObjectItemNumber(v)));
  return not_exist;
}

bool AmfObjectItemMap::put(const std::string &name, const std::string &v) {
  bool not_exist = true;
  std::map<std::string, AmfObjectItem *>::iterator iter = objs_.find(name);
  if (iter != objs_.end()) {
    objs_.erase(iter);
    not_exist = false;
  }
  objs_.insert(std::make_pair(name, new AmfObjectItemString(v)));
  return not_exist;
}

bool AmfObjectItemMap::put(const std::string &name, char *v, int len) {
  bool not_exist = true;
  std::map<std::string, AmfObjectItem *>::iterator iter = objs_.find(name);
  if (iter != objs_.end()) {
    objs_.erase(iter);
    not_exist = false;
  }
  objs_.insert(std::make_pair(name, new AmfObjectItemString(std::string(v, len))));
  return not_exist;
}

AmfObjectItem *AmfObjectItemMap::get(const std::string &name) {
  std::map<std::string, AmfObjectItem *>::iterator iter = objs_.find(name);
  if (iter == objs_.end()) {
    return NULL;
  }
  return iter->second;
}

std::string AmfObjectItemMap::stringify() {
  std::ostringstream oss;
  std::map<std::string, AmfObjectItem *>::iterator iter = objs_.begin();
  for (; iter != objs_.end(); iter++) {
    oss << "{<" << iter->first << ">:"
        << "<" << iter->second->stringify() << ">}";
  }
  return oss.str();

}

void AmfObjectItemMap::clear() {
  std::map<std::string, AmfObjectItem *>::iterator iter = objs_.begin();
  for (; iter != objs_.end(); iter++) {
    delete iter->second;
  }
  objs_.clear();
}

} // namespace yet