// -*- mode: c++; c-basic-offset: 4 -*-
/*
 * polldevice.{cc,hh} -- element reads packets live from network via Intel DPDK
 * multi-queue enabled, works with multi-queue 1G and 10G NICs
 * Douglas S. J. De Couto, Eddie Kohler, John Jannotti
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2001 International Computer Science Institute
 * Copyright (c) 2005-2007 Regents of the University of California
 * Copyright (c) 2011 Meraki, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */
/* Created 2012-2013 by Maziar Manesh, Intel Labs */

#include <click/config.h>
#include <sys/types.h>
#include <sys/time.h>
#include <click/router.hh>
#include <iostream>
#include <click/master.hh>
#if !defined(__sun)
# include <sys/ioctl.h>
#else
# include <sys/ioccom.h>
#endif

#include <click/error.hh>
#include <click/straccum.hh>
#include <click/args.hh>
#include <click/glue.hh>
#include <click/packet_anno.hh>
#include <click/standard/scheduleinfo.hh>
#include <click/userutils.hh>
#include <unistd.h>
#include <fcntl.h>
#include "fakepcap.hh"

#include "polldevice.hh"

/* Configure how many packets ahead to prefetch, when reading packets */
#define PREFETCH_OFFSET 3

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t stats_tx_on[];

#ifdef __cplusplus
}
#endif

CLICK_DECLS

PollDevice::PollDevice()
    :
     _d_task(this),_datalink(-1), _count(0), _promisc(0), _snaplen(0)
{
#if POLLDEVICE_LINUX || POLLDEVICE_PCAP
    _fd = -1;
#endif
}


PollDevice::~PollDevice()
{
}

int
PollDevice::configure(Vector<String> &conf, ErrorHandler *errh)
{
    bool promisc = false, outbound = false, sniffer = true;
    _snaplen = default_snaplen;
    _headroom = Packet::default_headroom;
    _headroom += (4 - (_headroom + 2) % 4) % 4; // default 4/2 alignment
    _force_ip = false;
    _burst = 1;
    _portid=0;
    _queueid=0;
    String bpf_filter, capture, encap_type;
    bool has_encap;
    if (Args(conf, this, errh)
	.read_mp("DEVNAME", _ifname)
	.read_p("PROMISC", promisc)
	.read_p("SNAPLEN", _snaplen)
	.read("SNIFFER", sniffer)
	.read("FORCE_IP", _force_ip)
	//.read("METHOD", WordArg(), capture)
	.read("CAPTURE", WordArg(), capture) // deprecated
	//.read("BPF_FILTER", bpf_filter)
	.read("OUTBOUND", outbound)
	.read("HEADROOM", _headroom)
	.read("ENCAP", WordArg(), encap_type).read_status(has_encap)
	.read("BURST", _burst)
	//.read("PORTID", _portid)
	.read("QUEUE", _queueid)
	.complete() < 0)
	return -1;
    if (_snaplen > 8190 || _snaplen < 14)
	return errh->error("SNAPLEN out of range");
    if (_headroom > 8190)
	return errh->error("HEADROOM out of range");
    if (_burst <= 0 || _queueid < 0)
	return errh->error("BURST or PORTID or QUEUE out of range");
    if (get_portID(_ifname) < 0)
	return errh->error("DEVNAME is not a valid device and not configured for use");

    click_chatter("PollDevice(%s): DPDK port_id: %d, burst: %d", _ifname.c_str(), get_portID(_ifname), _burst);
    
#if POLLDEVICE_PCAP
    _bpf_filter = bpf_filter;
    if (has_encap) {
        _datalink = fake_pcap_parse_dlt(encap_type);
        if (_datalink < 0)
            return errh->error("bad encapsulation type");
    }
#endif

    /*
    // set _capture
    if (capture == "") {
#if POLLDEVICE_PCAP && POLLDEVICE_LINUX
	_capture = CAPTURE_PCAP;
#elif POLLDEVICE_LINUX
	_capture = CAPTURE_LINUX;
#elif POLLDEVICE_PCAP
	_capture = CAPTURE_PCAP;
#else
	//return errh->error("cannot receive packets on this platform");
#endif
    }
#if POLLDEVICE_LINUX
    else if (capture == "LINUX")
	_capture = CAPTURE_LINUX;
#endif
#if POLLDEVICE_PCAP
    else if (capture == "PCAP")
	_capture = CAPTURE_PCAP;
#endif
    else
	return errh->error("bad METHOD");

    if (bpf_filter && _capture != CAPTURE_PCAP)
	errh->warning("not using METHOD PCAP, BPF filter ignored");
    */

    _sniffer = sniffer;
    _promisc = promisc;
    _outbound = outbound;
    return 0;
}

#if POLLDEVICE_LINUX
int
PollDevice::open_packet_socket(String ifname, ErrorHandler *errh)
{
    int fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd == -1)
	return errh->error("%s: socket: %s", ifname.c_str(), strerror(errno));

    // get interface index
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));
    int res = ioctl(fd, SIOCGIFINDEX, &ifr);
    if (res != 0) {
	close(fd);
	return errh->error("%s: SIOCGIFINDEX: %s", ifname.c_str(), strerror(errno));
    }
    int ifindex = ifr.ifr_ifindex;

    // bind to the specified interface.  from packet man page, only
    // sll_protocol and sll_ifindex fields are used; also have to set
    // sll_family
    sockaddr_ll sa;
    memset(&sa, 0, sizeof(sa));
    sa.sll_family = AF_PACKET;
    sa.sll_protocol = htons(ETH_P_ALL);
    sa.sll_ifindex = ifindex;
    res = bind(fd, (struct sockaddr *)&sa, sizeof(sa));
    if (res != 0) {
	close(fd);
	return errh->error("%s: bind: %s", ifname.c_str(), strerror(errno));
    }

    // nonblocking I/O on the packet socket so we can poll
    fcntl(fd, F_SETFL, O_NONBLOCK);

    return fd;
}

int
PollDevice::set_promiscuous(int fd, String ifname, bool promisc)
{
    // get interface flags
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) != 0)
	return -2;
    int was_promisc = (ifr.ifr_flags & IFF_PROMISC ? 1 : 0);

    // set or reset promiscuous flag
#ifdef SOL_PACKET
    if (ioctl(fd, SIOCGIFINDEX, &ifr) != 0)
	return -2;
    struct packet_mreq mr;
    memset(&mr, 0, sizeof(mr));
    mr.mr_ifindex = ifr.ifr_ifindex;
    mr.mr_type = (promisc ? PACKET_MR_PROMISC : PACKET_MR_ALLMULTI);
    if (setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0)
	return -3;
#else
    if (was_promisc != promisc) {
	ifr.ifr_flags = (promisc ? ifr.ifr_flags | IFF_PROMISC : ifr.ifr_flags & ~IFF_PROMISC);
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0)
	    return -3;
    }
#endif

    return was_promisc;
}
#endif /* POLLDEVICE_LINUX */

#if POLLDEVICE_PCAP
const char *
PollDevice::pcap_error(pcap_t *pcap, const char *ebuf)
{
    if ((!ebuf || !ebuf[0]) && pcap)
	ebuf = pcap_geterr(pcap);
    if (!ebuf || !ebuf[0])
	return "unknown error";
    else
	return ebuf;
}

pcap_t *
PollDevice::open_pcap(String ifname, int snaplen, bool promisc,
		      ErrorHandler *errh)
{
    char ebuf[PCAP_ERRBUF_SIZE];
    ebuf[0] = 0;
    pcap_t *pcap = pcap_open_live(ifname.mutable_c_str(), snaplen, promisc,
			       1,     /* timeout: don't wait for packets */
			       ebuf);

    // Note: pcap error buffer will contain the interface name
    if (!pcap) {
	errh->error("%s while opening %s", pcap_error(0, ebuf), ifname.c_str());
	return 0;
    } else if (ebuf[0])
	errh->warning("%s", ebuf);

    // nonblocking I/O on the packet socket so we can poll
# if HAVE_PCAP_SETNONBLOCK
    ebuf[0] = 0;
    if (pcap_setnonblock(pcap, 1, ebuf) < 0 || ebuf[0])
	errh->warning("pcap_setnonblock: %s", pcap_error(pcap, ebuf));
# else
    if (fcntl(pcap_fileno(pcap), F_SETFL, O_NONBLOCK) < 0)
	errh->warning("setting nonblocking: %s", strerror(errno));
# endif

    return pcap;
}
#endif

int
PollDevice::initialize(ErrorHandler *errh)
{
    if (!_ifname)
	return errh->error("interface not set");

    //getting the dpdk portid from dev-name
    _portid = get_portID(_ifname);

    /*unsetting the tx stats for this port*/
    //if (_queueid == 0)
       stats_tx_on[_portid] = 0;
    
    ScheduleInfo::initialize_task(this, &_d_task, true, errh);
    

#if POLLDEVICE_LINUX
    if (_capture == CAPTURE_LINUX) {
	_fd = open_packet_socket(_ifname, errh);
	if (_fd < 0)
	    return -1;

	int promisc_ok = set_promiscuous(_fd, _ifname, _promisc);
	if (promisc_ok < 0) {
	    if (_promisc)
		errh->warning("cannot set promiscuous mode");
	    _was_promisc = -1;
	} else
	    _was_promisc = promisc_ok;

	add_select(_fd, SELECT_READ);

	_datalink = FAKE_DLT_EN10MB;
    }
#endif
/*
    if (!_sniffer)
	if (KernelFilter::device_filter(_ifname, true, errh) < 0)
	    _sniffer = true;
*/
    return 0;
}

void
PollDevice::cleanup(CleanupStage stage)
{
    //  if (stage >= CLEANUP_INITIALIZED && !_sniffer)
    //	KernelFilter::device_filter(_ifname, false, ErrorHandler::default_handler());
#if POLLDEVICE_LINUX
    if (_fd >= 0 && _capture == CAPTURE_LINUX) {
	if (_was_promisc >= 0)
	    set_promiscuous(_fd, _ifname, _was_promisc);
	close(_fd);
    }
#endif
#if POLLDEVICE_PCAP
    if (_pcap)
	pcap_close(_pcap);
    _pcap = 0;
#endif
#if POLLDEVICE_PCAP || POLLDEVICE_LINUX
    _fd = -1;
#endif
}

#if POLLDEVICE_PCAP
CLICK_ENDDECLS
extern "C" {
void
PollDevice_get_packet(u_char* clientdata,
		      const struct pcap_pkthdr* pkthdr,
		      const u_char* data)
{
    static unsigned char bcast_addr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    PollDevice *fd = (PollDevice *) clientdata;
    int length = pkthdr->caplen;
    Packet *p = Packet::make(fd->_headroom, data, length, 0);

    // set packet type annotation
    if (p->data()[0] & 1) {
	if (memcmp(bcast_addr, p->data(), 6) == 0)
	    p->set_packet_type_anno(Packet::BROADCAST);
	else
	    p->set_packet_type_anno(Packet::MULTICAST);
    }

    // set annotations
    p->set_timestamp_anno(Timestamp::make_usec(pkthdr->ts.tv_sec, pkthdr->ts.tv_usec));
    p->set_mac_header(p->data());
    SET_EXTRA_LENGTH_ANNO(p, pkthdr->len - length);

    if (!fd->_force_ip || fake_pcap_force_ip(p, fd->_datalink))
	fd->output(0).push(p);
    else
	fd->checked_output_push(1, p);
}
}
CLICK_DECLS
#endif
/*
void
PollDevice::selected(int, int)
{
#if POLLDEVICE_PCAP
    if (_capture == CAPTURE_PCAP) {
	// Read and push() at most one packet.
	int r = pcap_dispatch(_pcap, _burst, PollDevice_get_packet, (u_char *) this);
	if (r > 0) {
	    _count += r;
	    _pcap_task.reschedule();
	} else if (r < 0 && ++_pcap_complaints < 5)
	    ErrorHandler::default_handler()->error("%{element}: %s", this, pcap_geterr(_pcap));
    }
#endif
#if POLLDEVICE_LINUX
    int nlinux = 0;
    while (_capture == CAPTURE_LINUX && nlinux < _burst) {
	struct sockaddr_ll sa;
	socklen_t fromlen = sizeof(sa);
	WritablePacket *p = Packet::make(_headroom, 0, _snaplen, 0);
	int len = recvfrom(_fd, p->data(), p->length(), MSG_TRUNC, (sockaddr *)&sa, &fromlen);
	if (len > 0 && (sa.sll_pkttype != PACKET_OUTGOING || _outbound)) {
	    if (len > _snaplen) {
		assert(p->length() == (uint32_t)_snaplen);
		SET_EXTRA_LENGTH_ANNO(p, len - _snaplen);
	    } else
		p->take(_snaplen - len);
	    p->set_packet_type_anno((Packet::PacketType)sa.sll_pkttype);
	    p->timestamp_anno().set_timeval_ioctl(_fd, SIOCGSTAMP);
	    p->set_mac_header(p->data());
	    ++nlinux;
	    ++_count;
	    if (!_force_ip || fake_pcap_force_ip(p, _datalink))
		output(0).push(p);
	    else
		checked_output_push(1, p);
	} else {
	    p->kill();
	    if (len <= 0 && errno != EAGAIN)
		click_chatter("PollDevice(%s): recvfrom: %s", _ifname.c_str(), strerror(errno));
	    break;
	}
    }
#endif
}
*/

//destructor for the userlevel packet
/*void dest (unsigned char * data, size_t len) 
{
    //do nothing
}
*/
bool
PollDevice::run_task(Task *)
{
  //std::cout << "RUNNING POLLDEVICE " << std::endl;
    struct rte_mbuf *m;
    //unsigned lcore_id;
    unsigned i, j, nb_rx, ret;

    //lcore_id = rte_lcore_id();

    nb_rx = rte_eth_rx_burst((uint8_t) _portid,  _queueid, pkts_burst, _burst);
	_count += nb_rx;
	  //std::cout << "GRABBED " << nb_rx << " PACKETS " << std::endl;
  if (nb_rx > 0) {
#if 1
	    for (j = 0; j < nb_rx; j++) {
		    m = pkts_burst[j];
		//this would be if a click pkt is created
		//*Packet *p = Packet::make(this->_headroom, rte_pktmbuf_mtod(m, u_char*), rte_pktmbuf_pkt_len(m), 0); 
		//*rte_pktmbuf_free(m);
		
		//send the entire mbuf, copies mbuf
		//Packet *p = Packet::make(this->_headroom, (u_char*) m, MBUF_SIZE, 0);
		
		//send the entire mbuf, doesn't copy mbuf, only a wrapper around it
		//Packet *p = Packet::make((u_char*) m, MBUF_SIZE, &dest);
            
		//must also consider send m pointer itseld for mbuf although the macro based on buf_add, head can be used
    Packet *p = Packet::make(rte_pktmbuf_mtod(m, u_char*), rte_pktmbuf_pkt_len(m), (u_char*) m->buf_addr, m->buf_len, &Packet::dest, true);
    //ret = rte_eth_tx_burst((uint8_t) 1, 0, pkts_burst, (uint16_t) nb_rx);
		//click_chatter("PollDevice(%s): recvpkts: %d", _ifname.c_str(), _count);
		//if (_count < 33)
		//    click_chatter("pktlen= %d, datalen: %d", rte_pktmbuf_pkt_len(m), rte_pktmbuf_data_len(m));
		this->output(0).push(p);
	    }
	    _d_task.reschedule();
#endif
	}
	_d_task.reschedule();
	return nb_rx > 0;
    
	// Read and push() at most one packet.
    /*
    int r = pcap_dispatch(_pcap, _burst, PollDevice_get_packet, (u_char *) this);
    if (r > 0) {
	_count += r;
	_pcap_task.fast_reschedule();
    } else if (r < 0 && ++_pcap_complaints < 5)
	ErrorHandler::default_handler()->error("%{element}: %s", this, pcap_geterr(_pcap));
    return r > 0;
    */
}

/*
void
PollDevice::kernel_drops(bool& known, int& max_drops) const
{
#if POLLDEVICE_LINUX
    // You might be able to do this better by parsing netstat/ifconfig output,
    // but for now, we just give up.
#endif
    known = false, max_drops = -1;
#if POLLDEVICE_PCAP
    if (_capture == CAPTURE_PCAP) {
	struct pcap_stat stats;
	if (pcap_stats(_pcap, &stats) >= 0)
	    known = true, max_drops = stats.ps_drop;
    }
#endif
}
*/
String
PollDevice::read_handler(Element* e, void *thunk)
{
    PollDevice* fd = static_cast<PollDevice*>(e);

		return String(fd->_count);
}

int
PollDevice::write_handler(const String &, Element *e, void *, ErrorHandler *)
{
    PollDevice* fd = static_cast<PollDevice*>(e);
    fd->_count = 0;
    return 0;
}

void
PollDevice::add_handlers()
{
    //add_read_handler("kernel_drops", read_handler, 0);
    //add_read_handler("encap", read_handler, 1);
    add_read_handler("count", read_handler, 0);
    add_write_handler("reset_counts", write_handler, 0, Handler::BUTTON);
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(userlevel)
EXPORT_ELEMENT(PollDevice)
