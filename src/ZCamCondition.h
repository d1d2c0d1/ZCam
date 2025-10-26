#pragma once
#include <Arduino.h>
#include "ZCamTypes.h"

class ZCamCondition {
public:
  enum Op : uint8_t { GT, GE, LT, LE, EQ, NE };

  struct Pred {
    Op op;
    int16_t value;
  };

  // Разобрать строку вида ">1200&<=1500&!1350"
  bool parse(const ZCamString& expr);

  // Проверить значение val по всем предикатам (AND)
  bool test(int16_t val) const;

private:
  static const uint8_t kMaxPreds = 8;
  Pred _preds[kMaxPreds];
  uint8_t _count = 0;

  bool addPred(Op op, int16_t v);
};
