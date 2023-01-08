#include <cstdint>
#include <cstdio>
#include <cmath>
#include <limits>
#include <string_view>

enum class FloatStatus : uint8_t {
    Normal=0, Denorm, Infinity, QuietNan, SignalNan
};

struct BinaryFloat {
    FloatStatus status;
    int64_t mantissa;  // mantissa
    int32_t exponent2; // power of two exponent
};

struct [[gnu::packed]] PackedDouble {
    uint64_t fraction: 52;
    uint32_t exponent: 11;
    uint32_t sign: 1;
};

BinaryFloat unpackDouble( double value ) {
    union {
        double dvalue;
        PackedDouble pack;
    };
    dvalue = value;
    BinaryFloat bin;
    if ( pack.exponent == 0x7ff ) { // infinity or quiet nan
        if ( pack.fraction == 0  ) {
            bin.status = FloatStatus::Infinity;
        } else if ( pack.fraction >> 51 == 1 ) {
            bin.status = FloatStatus::QuietNan;
        } else {
            bin.status = FloatStatus::SignalNan;
        }
    } else if ( pack.exponent == 0 ) { // denorm
        bin.status = FloatStatus::Denorm;
        bin.mantissa = pack.fraction;
    } else {
        bin.status = FloatStatus::Normal;
        bin.mantissa = (1ULL<<52) | pack.fraction;
    }
    if (pack.sign) bin.mantissa = -bin.mantissa;
    bin.exponent2 = pack.exponent - 1075;
    return bin;
}

void test( const char* name, double value ) {
    const char* StatusName[] = { "Normal", "Denorm", "Infinity", "QuietNaN", "SignalNan" };
    union {
        double dvalue;
        PackedDouble pack;
    };
    dvalue = value;
    BinaryFloat bin = unpackDouble( value );
    double check = bin.mantissa * pow(2,bin.exponent2);
    printf( "%10s  Number:%-13g Recovered:%13g   IEEE754:(%x,%-3x,%-13lx) "
            "  Mantissa:%17ld (0x%014lx)   Exponent:%+5d (0x%08x)  Status:%s\n", 
            name, value, check, pack.sign, pack.exponent, pack.fraction, 
            bin.mantissa, std::abs(bin.mantissa), bin.exponent2, bin.exponent2, StatusName[(int)bin.status] );
}

int main() {
    test("Zero",0);
    test("2",2);
    test("1",1);
    test("0.5",0.5);
    test("0.25",0.25);
    test("0.125",0.125);
    test("-2", -2 );
    test("-1", -1 );
    test("-0.5", -0.5 );
    test("-0.25", -0.25 );
    test("Pi",3.14);
    test("min",std::numeric_limits<double>::min());
    test("max",std::numeric_limits<double>::max());
    test("lowest",std::numeric_limits<double>::lowest());
    test("quiet_NaN",std::numeric_limits<double>::quiet_NaN());
    test("sig_NaN",std::numeric_limits<double>::signaling_NaN());
    test("infinity",std::numeric_limits<double>::infinity());
    test("rounderr",std::numeric_limits<double>::round_error());
    test("epsilon",std::numeric_limits<double>::epsilon());
    test("denorm",std::numeric_limits<double>::denorm_min());
    test("denorm",0.61E-323);
    return 0;
}
