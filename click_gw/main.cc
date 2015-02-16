#include "tree/utils.h"
#include "ope_tree.h"
#include <ostream>
#include <iterator>

std::ostream&
operator<<( std::ostream& dest, uint128_t value )
{
    std::ostream::sentry s( dest );
    if ( s ) {
        uint128_t tmp = value;
        char buffer[ 128 ];
        char* d = std::end(buffer);
        do
        {
            -- d;
            *d = "0123456789"[ tmp % 10 ];
            tmp /= 10;
        } while ( tmp != 0 );
        int len = std::end( buffer ) - d;
        if ( dest.rdbuf()->sputn( d, len ) != len ) {
            dest.setstate( std::ios_base::badbit );
        }
    }
    return dest;
}

int main()
{
    OPETree1<uint64_t> tree;
    tree.add(832423);
    tree.add(23948689);
    tree.add(8819290436560);

    assert(tree.size() == 3);

    auto range1 = tree.ciphertext_range(324);
    auto range2 = tree.ciphertext_range(832874);
    auto range3 = tree.ciphertext_range(881929043656);
    auto range4 = tree.ciphertext_range(8819290436565);

    std::cout << (range1.first) << " " << (range1.second) << std::endl;
    std::cout << range2.first << " " << range2.second << std::endl;
    std::cout << range3.first << " " << range3.second << std::endl;
    std::cout << range4.first << " " << range4.second << std::endl;

    for (int i = 0; i < 100; ++i) {
        std::cout << tree.generate_ciphertext(324) << std::endl;
    }

    return 0;
}