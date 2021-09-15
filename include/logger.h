#ifndef LOGGER_H
#define LOGGER_H
#include "print.h"

#define nameWrapper(t_name, T) \
  struct t_name {      \
    constexpr static const char* value0 = #t_name; \
    T val; \
  };

namespace store {
  template<class T, class U> struct is_same {
    static constexpr bool value = false;
  };

  template<class T> struct is_same<T, T> {
    static constexpr bool value = true;
  };

  template<typename T, typename ...Ts> struct setTuple;

  namespace detail {
    template <typename T, typename ...Ts, typename TT, typename gen::enable_if<!is_same<TT, T>::value, int>::type = 0> void set_impl(setTuple<T, Ts...>& sT, const TT& v) {
      sT.vals.template set<TT>(v);
    }

    template<typename T> void set_impl(T& sT, const decltype(T::val)& v) {
      sT.val = v;
    }
  }

  void write_log(File& file, const char* text) {
    printf(text);
    file.print(text);
  }

  template<typename T, typename ...Ts> struct setTuple {
    static constexpr size_t size() {
      return sizeof(T) + setTuple<Ts...>::size();
    }

    T val{0};
    setTuple<Ts...> vals;
    
    template<typename TT> void set(const TT& v) {
      detail::set_impl(*this, v);
    }

    void names(File& f) const {
      write_log(f, T::value0);
      write_log(f, ",");
      vals.names(f);
    }

    void values(File& f) const {
      write_log(f, String(val.val).c_str());
      write_log(f, ",");
      vals.values(f);
    }
  } __attribute__((packed));

  template<typename T> struct setTuple<T> {
    static constexpr size_t size() {
      return sizeof(T);
    }

    T val{0};

    template<typename TT> void set(const TT& v) {
      val = v;
    }

    void names(File& f) const {
      write_log(f, T::value0);
      write_log(f, "\n");
    }

    void values(File& f) const {
      write_log(f, String(val.val).c_str());
      write_log(f, "\n");
    }
  } __attribute__((packed));

  nameWrapper(ms, uint32) //stm32
  nameWrapper(ch4, float) //mq-4
  nameWrapper(co, float) //mq-7
  nameWrapper(o3, float) //mq-131
  nameWrapper(tIntBMP, float) //bmp280
  nameWrapper(p, float)
  nameWrapper(altBMP, float)
  nameWrapper(tExt, float) //sht21
  nameWrapper(hum, float)
  nameWrapper(pm1, float) //sps30
  nameWrapper(pm2_5, float)
  nameWrapper(pm4, float)
  nameWrapper(pm10, float)
  nameWrapper(nc0_5, float)
  nameWrapper(nc1, float)
  nameWrapper(nc2_5, float)
  nameWrapper(nc4, float)
  nameWrapper(nc10, float)
  nameWrapper(avgP, float)
  nameWrapper(lng, int32_t) //max-m8q
  nameWrapper(lat, int32_t)
  nameWrapper(altGPS, float)
  nameWrapper(heading, float)
  nameWrapper(spd, float)
  nameWrapper(t, uint32_t)

  using loggerType = setTuple<ms, ch4, co, o3, tIntBMP, p, altBMP, tExt, hum, pm1, pm2_5, pm4, pm10, nc0_5, nc1, nc2_5, nc4, nc10, avgP, lng, lat, altGPS, heading, spd, t>;
  //cannot be a union because anonymous unions are static at namespace scope to prevent collisions
  loggerType values;

  byte* arr = reinterpret_cast<byte*>(&store::values);

  static_assert(loggerType::size() == sizeof(loggerType), "Incorrect packing");
}
#endif
