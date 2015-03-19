#include <click/config.h>
#include <clicknet/ether.h>
#include <click/etheraddress.hh>
#include <click/ipaddress.hh>
#include <click/args.hh>
#include <click/bitvector.hh>
#include <click/straccum.hh>
#include <click/router.hh>
#include <click/error.hh>
#include <click/glue.hh>

#include "mbarktable.hh"

CLICK_DECLS

MBarkTable::MBarkTable()
{
}

MBarkTable::~MBarkTable()
{
}

void
MBarkTable::take_state(Element *e, ErrorHandler *)
{
    MBarkTable *mbarkt = (MBarkTable *)e->cast("MBarkTable");
    if (!mbarkt)
	   return;
    _table.swap(mbarkt->_table);
}


CLICK_ENDDECLS

EXPORT_ELEMENT(MBarkTable)
ELEMENT_MT_SAFE(MBarkTable)
