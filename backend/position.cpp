#include "position.h"

void getPosition(double A, double B, double C, double D, double &x, double &y) {
  // 计算x,y坐标
  if (A == 0) {
    getLimitPosition(B, C, D, x, y);
  } else if (B == 0) {
    getLimitPosition(C, D, A, x, y);
    auto t = y;
    y = x;
    x = -t;
  } else if (C == 0) {
    getLimitPosition(D, A, B, x, y);
    x = -x;
    y = -y;
  } else if (D == 0) {
    getLimitPosition(A, B, C, x, y);
    auto t = x;
    x = y;
    y = -t;
  }
}

void getLimitPosition(double B, double C, double D, double &x, double &y) {
  x = C * C / 2 + D * D / 2 - C * D +
      ((C - D) * (B - D + B * C * C + B * B * C + 2 * B * B * D -
                  3 * C * D * D + 5 * C * C * D -
                  sqrt(-(B - C + 1) * (C - B + 1) * (C - D + 1) * (D - C + 1) *
                       (B * B - 2 * B * D + D * D - 2)) -
                  B * B * B - 2 * C * C * C + D * D * D - 4 * B * C * D)) /
          (2 * (B * B - 2 * B * C + 2 * C * C - 2 * C * D + D * D - 1));
  y = C * C / 2 - B * B / 2 + B * D - C * D -
      (B * (B - D + B * C * C + B * B * C + 2 * B * B * D - 3 * C * D * D +
            5 * C * C * D -
            sqrt(-(B - C + 1) * (C - B + 1) * (C - D + 1) * (D - C + 1) *
                 (B * B - 2 * B * D + D * D - 2)) -
            B * B * B - 2 * C * C * C + D * D * D - 4 * B * C * D)) /
          (2 * (B * B - 2 * B * C + 2 * C * C - 2 * C * D + D * D - 1)) +
      (C * (B - D + B * C * C + B * B * C + 2 * B * B * D - 3 * C * D * D +
            5 * C * C * D -
            sqrt(-(B - C + 1) * (C - B + 1) * (C - D + 1) * (D - C + 1) *
                 (B * B - 2 * B * D + D * D - 2)) -
            B * B * B - 2 * C * C * C + D * D * D - 4 * B * C * D)) /
          (2 * (B * B - 2 * B * C + 2 * C * C - 2 * C * D + D * D - 1));
}