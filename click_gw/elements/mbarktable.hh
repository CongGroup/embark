#ifndef CLICK_MBARKTABLE_HH
#define CLICK_MBARKTABLE_HH

#include <click/element.hh>
#include <click/etheraddress.hh>
#include <click/hashtable.hh>
#include <click/sync.hh>
#include <click/timer.hh>
#include <click/list.hh>
#include <click/ip6flowid.hh>
#include <click/ipflowid.hh>

CLICK_DECLS

class MBarkTable : public Element { public:

    MBarkTable();
    ~MBarkTable();

    const char *class_name() const		{ return "MBarkTable"; }
    bool can_live_reconfigure() const		{ return true; }
    void take_state(Element *, ErrorHandler *);

    IP6FlowID lookup(const IP6FlowID& after);
    IPFlowID lookup(const IPFlowID& after);
    void insert(const IP6FlowID& before, const IP6FlowID& after);
    void insert(const IPFlowID& before, const IPFlowID& after);

  private:
    ReadWriteLock _lock;
    HashTable<IP6FlowID, IP6FlowID> _table;
    HashTable<IPFlowID, IPFlowID> _table_v4;
};

inline IP6FlowID
MBarkTable::lookup(const IP6FlowID& after)
{
    _lock.acquire_read();
    IP6FlowID r;
    if (auto it = _table.find(after)) {
        r = it->second;
    }
    _lock.release_read();
    return r;
}

inline IPFlowID
MBarkTable::lookup(const IPFlowID& after)
{
    _lock.acquire_read();
    IPFlowID r;
    if (auto it = _table_v4.find(after)) {
        r = it->second;
    }
    _lock.release_read();
    return r;
}

inline void
MBarkTable::insert(const IP6FlowID& before, const IP6FlowID& after)
{
    _lock.acquire_write();
    _table[before] = after;
    _lock.release_write();
}

inline void
MBarkTable::insert(const IPFlowID& before, const IPFlowID& after)
{
    _lock.acquire_write();
    _table_v4[before] = after;
    _lock.release_write();
}

CLICK_ENDDECLS
#endif
