#include "ZCamCondition.h"

bool ZCamCondition::addPred(Op op, int16_t v) {
  if (_count >= kMaxPreds) return false;
  _preds[_count++] = Pred{op, v};
  return true;
}

bool ZCamCondition::parse(const ZCamString& expr) {
  _count = 0;
  int start = 0;
  while (start < expr.length()) {
    int amp = expr.indexOf('&', start);
    ZCamString tok = (amp < 0) ? expr.substring(start) : expr.substring(start, amp);
    tok.trim();

    if (tok.length() == 0) { if (amp < 0) break; start = amp + 1; continue; }

    Op op = EQ;
    int16_t val = 0;

    if (tok.startsWith(">=")) { op = GE; val = tok.substring(2).toInt(); }
    else if (tok.startsWith("<=")) { op = LE; val = tok.substring(2).toInt(); }
    else if (tok.startsWith(">"))  { op = GT; val = tok.substring(1).toInt(); }
    else if (tok.startsWith("<"))  { op = LT; val = tok.substring(1).toInt(); }
    else if (tok.startsWith("!"))  { op = NE; val = tok.substring(1).toInt(); }
    else                           { op = EQ; val = tok.toInt(); }

    if (!addPred(op, val)) return false;

    if (amp < 0) break;
    start = amp + 1;
  }
  return true;
}

bool ZCamCondition::test(int16_t v) const {
  for (uint8_t i = 0; i < _count; ++i) {
    const auto &p = _preds[i];
    bool ok = false;
    switch (p.op) {
      case GT: ok = (v >  p.value); break;
      case GE: ok = (v >= p.value); break;
      case LT: ok = (v <  p.value); break;
      case LE: ok = (v <= p.value); break;
      case EQ: ok = (v == p.value); break;
      case NE: ok = (v != p.value); break;
    }
    if (!ok) return false;
  }
  return true;
}
