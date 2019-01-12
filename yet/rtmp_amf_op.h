/**
 * @file   rtmp_amf_op.h
 * @author pengrl
 *
 */

#pragma once

#include <inttypes.h>
#include <assert.h>
#include <map>
#include <string>
#include <sstream>

namespace yet {

  enum Amf0DataType {
    Amf0DataType_NUMBER       = 0x00,
    Amf0DataType_BOOLEAN      = 0x01,
    Amf0DataType_STRING       = 0x02,
    Amf0DataType_OBJECT       = 0x03,
    Amf0DataType_MOVIECLIP    = 0x04,
    Amf0DataType_NULL         = 0x05,
    Amf0DataType_UNDEFINED    = 0x06,
    Amf0DataType_REFERENCE    = 0x07,
    Amf0DataType_ECMA_ARRAY   = 0x08,
    Amf0DataType_OBJECT_END   = 0x09,
    Amf0DataType_STRICT_ARRAY = 0x0A,
    Amf0DataType_DATE         = 0x0B,
    Amf0DataType_LONG_STRING  = 0x0C,
    Amf0DataType_UNSUPPORTED  = 0x0D,
    Amf0DataType_RECORDSET    = 0x0E,
    Amf0DataType_XML_DOCUMENT = 0x0F,
    Amf0DataType_TYPED_OBJECT = 0x10,
  }; // enum Amf0DataType

  enum Amf0ObjectValueType {
    Amf0ObjectValueType_NUMBER  = Amf0DataType_NUMBER,
    Amf0ObjectValueType_BOOLEAN = Amf0DataType_BOOLEAN,
    Amf0ObjectValueType_STRING  = Amf0DataType_STRING,
  }; // enum Amf0ObjectValueType

  class AmfObjectItem {
    public:
      AmfObjectItem(Amf0ObjectValueType type) : type_(type) {}
      virtual ~AmfObjectItem() {}
      bool is_boolean() { return type_ == Amf0ObjectValueType_BOOLEAN; }
      bool is_number() { return type_ == Amf0ObjectValueType_NUMBER; }
      bool is_string() { return type_ == Amf0ObjectValueType_STRING; }
      virtual bool get_boolean() { assert(0);return false; }
      virtual double get_number() { assert(0);return 0; }
      virtual std::string get_string() { assert(0);return std::string(); }
      virtual void set_boolean(bool) { assert(0); }
      virtual void set_number(double) { assert(0); }
      virtual void set_string(const std::string &) { assert(0); }
      virtual std::string stringify() { assert(0);return std::string(); }

    private:
      Amf0ObjectValueType type_;
  }; // class AmfObjectItem

  class AmfObjectItemBoolean : public AmfObjectItem {
    public:
      AmfObjectItemBoolean(bool v) : AmfObjectItem(Amf0ObjectValueType_BOOLEAN), v_(v) {}
      virtual bool get_boolean() { return v_; }
      virtual void set_boolean(bool v) { v_ = v; }
      virtual std::string stringify() { std::ostringstream oss; oss << v_; return oss.str(); }
    private:
      bool v_;
  }; // class AmfObjectItemBoolean

  class AmfObjectItemNumber: public AmfObjectItem {
    public:
      AmfObjectItemNumber(double v) : AmfObjectItem(Amf0ObjectValueType_NUMBER), v_(v) {}
      virtual double get_number() { return v_; }
      virtual void set_number(double v) { v_ = v; }
      virtual std::string stringify() { std::ostringstream oss; oss << v_; return oss.str(); }
    private:
      double v_;
  }; // class AmfObjectItemNumber

  class AmfObjectItemString: public AmfObjectItem {
    public:
      AmfObjectItemString(const std::string &v) : AmfObjectItem(Amf0ObjectValueType_STRING), v_(v) {}
      AmfObjectItemString(const char *data, int len) : AmfObjectItem(Amf0ObjectValueType_STRING), v_(data, len) {}
      virtual std::string get_string() { return v_; }
      virtual void set_string(const std::string &v) { v_ = v; }
      virtual std::string stringify() { std::ostringstream oss; oss << v_; return oss.str(); }
    private:
      std::string v_;
  }; // class AmfObjectItemNumber

  class AmfObjectItemMap {
    public:
      AmfObjectItemMap();
      ~AmfObjectItemMap();

      // 插入前<name>不存在则返回true，存在返回false。注意，如果已存在，会进行覆盖
      bool put(const std::string &name, bool v);
      bool put(const std::string &name, double v);
      bool put(const std::string &name, const std::string &v);
      bool put(const std::string &name, const char *v, int len);

      // 存在则返回，不存在返回NULL
      AmfObjectItem *get(const std::string &name);

      std::size_t size() const { return objs_.size(); }
      std::string stringify();

      void clear();

    private:
      std::map<std::string, AmfObjectItem *> objs_;
  }; // class AmfObjectItemMap

  class AmfOp {
    public:
      static constexpr int ENCODE_INT16_RESERVE = 2;
      static constexpr int ENCODE_INT24_RESERVE = 3;
      static constexpr int ENCODE_INT32_RESERVE = 4;
      static constexpr int ENCODE_NUMBER_RESERVE = 9; /// with type
      static constexpr int ENCODE_BOOLEAN_RESERVE = 2; /// with type
      static constexpr int ENCODE_NULL_RESERVE = 1;
      static constexpr int ENCODE_OBJECT_BEGIN_RESERVE = 1;
      static constexpr int ENCODE_OBJECT_END_RESERVE = 3;
      static constexpr int ENCODE_ECMA_ARRAY_BEGIN_RESERVE = 5;
      static constexpr int ENCODE_ECMA_ARRAY_END_RESERVE = 3;

      static int encode_int16_reserve() { return ENCODE_INT16_RESERVE; }
      static int encode_int24_reserve() { return ENCODE_INT24_RESERVE; }
      static int encode_int32_reserve() { return ENCODE_INT32_RESERVE; }
      static int encode_number_reserve() { return ENCODE_NUMBER_RESERVE; }
      static int encode_boolean_reserve() { return ENCODE_BOOLEAN_RESERVE; }
      static int encode_null_reserve() { return ENCODE_NULL_RESERVE; }
      static int encode_object_begin_reserve() { return ENCODE_OBJECT_BEGIN_RESERVE; }
      static int encode_object_end_reserve() { return ENCODE_OBJECT_END_RESERVE; }
      static int encode_ecma_array_begin_reserve() { return ENCODE_ECMA_ARRAY_BEGIN_RESERVE; }
      static int encode_ecma_array_end_reserve() { return ENCODE_ECMA_ARRAY_END_RESERVE; }
      static int encode_string_reserve(int val_len) { return (val_len < 65536) ? (val_len+1+2) : (val_len+1+4); }
      static int encode_object_named_boolean_reserve(int name_len) { return 2+name_len+ENCODE_BOOLEAN_RESERVE; }
      static int encode_object_named_number_reserve(int name_len) { return 2+name_len+ENCODE_NUMBER_RESERVE; }
      static int encode_object_named_string_reserve(int name_len, int val_len) {
        return 2+name_len+encode_string_reserve(val_len);
      }

    public:
      /**
       * @function encode_xxx
       * @param out 空间由外部申请,调用前需确保有对应所需的剩余空间,可以使用上面提供的xxx_reserve系列函数
       * @return 成功则返回<out>填入编码数据后的向后偏移后的位置，失败则返回NULL
       * @NOTICE 函数内部不会修改待编码数据
       *
       */

      static uint8_t *encode_int16(uint8_t *out, int16_t val);
      static uint8_t *encode_int24(uint8_t *out, int32_t val);
      static uint8_t *encode_int32(uint8_t *out, int32_t val);
      static uint8_t *encode_int32_le(uint8_t *out, int32_t val);
      static uint8_t *encode_number(uint8_t *out, double val);
      static uint8_t *encode_boolean(uint8_t *out, bool val);
      static uint8_t *encode_null(uint8_t *out);
      static uint8_t *encode_string(uint8_t *out, const char *val, int val_len);

      static uint8_t *encode_object_begin(uint8_t *out);
      static uint8_t *encode_object_named_boolean(uint8_t *out, const char *name, int name_len, bool val);
      static uint8_t *encode_object_named_number(uint8_t *out, const char *name, int name_len, double val);
      static uint8_t *encode_object_named_string(uint8_t *out, const char *name, int name_len, const char *val, int val_len);
      static uint8_t *encode_object_end(uint8_t *out);

      static uint8_t *encode_ecma_array_begin(uint8_t *out, int array_len);
      static uint8_t *encode_ecma_array_named_boolean(uint8_t *out, const char *name, int name_len, double val) {
        return encode_object_named_boolean(out, name, name_len, val);
      }
      static uint8_t *encode_ecma_array_named_number(uint8_t *out, const char *name, int name_len, double val) {
        return encode_object_named_number(out, name, name_len, val);
      }
      static uint8_t *encode_ecma_array_named_string(uint8_t *out, const char *name, int name_len, const char *val, int val_len) {
        return encode_object_named_string(out, name, name_len, val, val_len);
      }
      static uint8_t *encode_ecma_array_end(uint8_t *out);

    public:
      /**
       * @function decode_xxx
       * @param in 内部不会修改<in>内容
       * @param valid_len 当前从<in>开始有效数据大小，如果该值小于解码需要的大小，则解码失败
       * @param out 解码结果，如无特殊声明，则内存由外部提供
       * @param used_len 解码使用了<in>多少字节，如果填入NULL，则表示不获取
       * @return 成功则返回解码后<in>向后偏移的位置，和<used_len>提供的功能类似，失败则返回NULL
       *
       */

      static uint8_t *decode_boolean_with_type(const uint8_t *in, int valid_len, bool *out, int *used_len);
      static uint8_t *decode_number_with_type(const uint8_t *in, int valid_len, double *out, int *used_len);
      static uint8_t *decode_int16(const uint8_t *in, int valid_len, int32_t *out, int *used_len);
      static uint8_t *decode_int24(const uint8_t *in, int valid_len, int32_t *out, int *used_len);
      static uint8_t *decode_int32(const uint8_t *in, int valid_len, int32_t *out, int *used_len);
      static uint8_t *decode_int32_le(const uint8_t *in, int valid_len, int32_t *out, int *used_len);


      // @NOTICE <out>指向<in>中某个位置，没有拷贝生成新内存
      static uint8_t *decode_string(const uint8_t *in, int valid_len, char **out, int *str_len, int *used_len);
      static uint8_t *decode_string_with_type(const uint8_t *in, int valid_len, char **out, int *str_len, int *used_len);

      static uint8_t *decode_object(const uint8_t *in, int valid_len, AmfObjectItemMap *objs, int *used_len);

    private:
      AmfOp() = delete;
      AmfOp(const AmfOp &) = delete;
      const AmfOp &operator=(const AmfOp &) = delete;

  }; // class AmfOp

}; // namespace yet
