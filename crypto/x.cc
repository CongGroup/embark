#include <vector>
#include <iomanip>
// #include <crypto/cbc.hh>
// #include <crypto/cmc.hh>
// #include <crypto/prng.hh>
// #include <crypto/aes.hh>
// #include <crypto/blowfish.hh>
#include <crypto/ope.hh>
// #include <crypto/arc4.hh>
// #include <crypto/hgd.hh>
// #include <crypto/sha.hh>
// #include <crypto/hmac.hh>
// #include <crypto/paillier.hh>
// #include <crypto/bn.hh>
// #include <crypto/ecjoin.hh>
// #include <crypto/search.hh>
// #include <crypto/skip32.hh>
// #include <crypto/cbcmac.hh>
// #include <crypto/ffx.hh>
// #include <crypto/online_ope.hh>
// #include <crypto/padding.hh>
// #include <crypto/mont.hh>
// #include <crypto/gfe.hh>
#include "util/timer.hh"
#include <NTL/ZZ.h>
#include <NTL/RR.h>

using namespace std;
using namespace NTL;


static void
test_ope(int pbits, int cbits)
{
    urandom u;
    OPE o("hello world", pbits, cbits);
    RR maxerr = to_RR(0);

    timer t;
    uint64_t enc = 0, dec = 0;
    enum { niter = 100 };
    for (uint i = 0; i < niter; i++) {
        ZZ pt = u.rand_zz_mod(to_ZZ(1) << pbits);
        t.lap();
        ZZ ct = o.encrypt(pt);
        enc += t.lap();
        ZZ pt2 = o.decrypt(ct);
        dec += t.lap();
        //throw_c(pt2 == pt);
        // cout << pt << " -> " << o.encrypt(pt, -1) << "/" << ct << "/" << o.encrypt(pt, 1) << " -> " << pt2 << endl;

        RR::SetPrecision(cbits+pbits);
        ZZ guess = ct / (to_ZZ(1) << (cbits-pbits));
        RR error = abs(to_RR(guess) / to_RR(pt) - 1);
        maxerr = max(error, maxerr);
        // cout << "pt guess is " << error << " off" << endl;
    }
    cout << "--- ope: " << pbits << "-bit plaintext, "
         << cbits << "-bit ciphertext" << endl
         << "  enc: " << enc / niter << " usec; "
         << "  dec: " << dec / niter << " usec; "
         << "~#bits leaked: "
           << ((maxerr < pow(to_RR(2), to_RR(-pbits))) ? pbits
                                                       : NumBits(to_ZZ(1/maxerr))) << endl;
}

int
main(int ac, char **av)
{
    //test_online_ope_rebalance();

    test_ope(32, 128);
}
