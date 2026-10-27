#include "shipping.h"

int shippingFee(int grams) {
    int fee = 8;
    if (grams > 1000) {
        int extra = (grams - 1000 + 499) / 500;
        fee = 8 + 3 * extra;
    }
    if (fee > 50) fee = 50;
    return fee;
}
