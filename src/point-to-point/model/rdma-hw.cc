#include <ns3/simulator.h>
#include <ns3/seq-ts-header.h>
#include <ns3/udp-header.h>
#include <ns3/ipv4-header.h>
#include <assert.h>
#include "ns3/ppp-header.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/data-rate.h"
#include "ns3/pointer.h"
#include "ns3/rdma-hw.h"
#include "ns3/ppp-header.h"
#include "ns3/qbb-header.h"
#include "ns3/cn-header.h"

namespace ns3{

/*[hyx]*/
// random extra pcie bandwidth
uint64_t GetRandomPCIeBW(){
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, 100);

	int rnum = dis(gen);
	if(rnum <= 12){
		return 0;  // 12% probability
	}

	int equal_prob = (rnum - 12) % 4;
	switch(equal_prob){
		case 0:
			return 1;  // 22% probability
		case 1:
			return 2;
		case 2:
			return 3;
		case 3:
			return 4;
		default:
			return 0;  // 0% probability, should not reach here
	}
};

TypeId RdmaHw::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::RdmaHw")
		.SetParent<Object> ()
		.AddAttribute("MinRate",
				"Minimum rate of a throttled flow",
				DataRateValue(DataRate("100Mb/s")),
				MakeDataRateAccessor(&RdmaHw::m_minRate),
				MakeDataRateChecker())
		.AddAttribute("Mtu",
				"Mtu.",
				UintegerValue(1000),
				MakeUintegerAccessor(&RdmaHw::m_mtu),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute ("CcMode",
				"which mode of DCQCN is running",
				UintegerValue(0),
				MakeUintegerAccessor(&RdmaHw::m_cc_mode),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("NACK Generation Interval",
				"The NACK Generation interval",
				DoubleValue(500.0),
				MakeDoubleAccessor(&RdmaHw::m_nack_interval),
				MakeDoubleChecker<double>())
		.AddAttribute("L2ChunkSize",
				"Layer 2 chunk size. Disable chunk mode if equals to 0.",
				UintegerValue(0),
				MakeUintegerAccessor(&RdmaHw::m_chunk),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("L2AckInterval",
				"Layer 2 Ack intervals. Disable ack if equals to 0.",
				UintegerValue(0),
				MakeUintegerAccessor(&RdmaHw::m_ack_interval),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("L2BackToZero",
				"Layer 2 go back to zero transmission.",
				BooleanValue(false),
				MakeBooleanAccessor(&RdmaHw::m_backto0),
				MakeBooleanChecker())
		.AddAttribute("EwmaGain",
				"Control gain parameter which determines the level of rate decrease",
				DoubleValue(1.0 / 16),
				MakeDoubleAccessor(&RdmaHw::m_g),
				MakeDoubleChecker<double>())
		.AddAttribute ("RateOnFirstCnp",
				"the fraction of rate on first CNP",
				DoubleValue(1.0),
				MakeDoubleAccessor(&RdmaHw::m_rateOnFirstCNP),
				MakeDoubleChecker<double> ())
		.AddAttribute("ClampTargetRate",
				"Clamp target rate.",
				BooleanValue(false),
				MakeBooleanAccessor(&RdmaHw::m_EcnClampTgtRate),
				MakeBooleanChecker())
		.AddAttribute("RPTimer",
				"The rate increase timer at RP in microseconds",
				DoubleValue(1500.0),
				MakeDoubleAccessor(&RdmaHw::m_rpgTimeReset),
				MakeDoubleChecker<double>())
		.AddAttribute("RateDecreaseInterval",
				"The interval of rate decrease check",
				DoubleValue(4.0),
				MakeDoubleAccessor(&RdmaHw::m_rateDecreaseInterval),
				MakeDoubleChecker<double>())
		.AddAttribute("FastRecoveryTimes",
				"The rate increase timer at RP",
				UintegerValue(5),
				MakeUintegerAccessor(&RdmaHw::m_rpgThreshold),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("AlphaResumInterval",
				"The interval of resuming alpha",
				DoubleValue(55.0),
				MakeDoubleAccessor(&RdmaHw::m_alpha_resume_interval),
				MakeDoubleChecker<double>())
		.AddAttribute("RateAI",
				"Rate increment unit in AI period",
				DataRateValue(DataRate("5Mb/s")),
				MakeDataRateAccessor(&RdmaHw::m_rai),
				MakeDataRateChecker())
		.AddAttribute("RateHAI",
				"Rate increment unit in hyperactive AI period",
				DataRateValue(DataRate("50Mb/s")),
				MakeDataRateAccessor(&RdmaHw::m_rhai),
				MakeDataRateChecker())
		.AddAttribute("VarWin",
				"Use variable window size or not",
				BooleanValue(false),
				MakeBooleanAccessor(&RdmaHw::m_var_win),
				MakeBooleanChecker())
		.AddAttribute("FastReact",
				"Fast React to congestion feedback",
				BooleanValue(true),
				MakeBooleanAccessor(&RdmaHw::m_fast_react),
				MakeBooleanChecker())
		.AddAttribute("MiThresh",
				"Threshold of number of consecutive AI before MI",
				UintegerValue(5),
				MakeUintegerAccessor(&RdmaHw::m_miThresh),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("TargetUtil",
				"The Target Utilization of the bottleneck bandwidth, by default 95%",
				DoubleValue(0.95),
				MakeDoubleAccessor(&RdmaHw::m_targetUtil),
				MakeDoubleChecker<double>())
		.AddAttribute("UtilHigh",
				"The upper bound of Target Utilization of the bottleneck bandwidth, by default 98%",
				DoubleValue(0.98),
				MakeDoubleAccessor(&RdmaHw::m_utilHigh),
				MakeDoubleChecker<double>())
		.AddAttribute("RateBound",
				"Bound packet sending by rate, for test only",
				BooleanValue(true),
				MakeBooleanAccessor(&RdmaHw::m_rateBound),
				MakeBooleanChecker())
		.AddAttribute("MultiRate",
				"Maintain multiple rates in HPCC",
				BooleanValue(true),
				MakeBooleanAccessor(&RdmaHw::m_multipleRate),
				MakeBooleanChecker())
		.AddAttribute("SampleFeedback",
				"Whether sample feedback or not",
				BooleanValue(false),
				MakeBooleanAccessor(&RdmaHw::m_sampleFeedback),
				MakeBooleanChecker())
		.AddAttribute("TimelyAlpha",
				"Alpha of TIMELY",
				DoubleValue(0.875),
				MakeDoubleAccessor(&RdmaHw::m_tmly_alpha),
				MakeDoubleChecker<double>())
		.AddAttribute("TimelyBeta",
				"Beta of TIMELY",
				DoubleValue(0.8),
				MakeDoubleAccessor(&RdmaHw::m_tmly_beta),
				MakeDoubleChecker<double>())
		.AddAttribute("TimelyTLow",
				"TLow of TIMELY (ns)",
				UintegerValue(50000),
				MakeUintegerAccessor(&RdmaHw::m_tmly_TLow),
				MakeUintegerChecker<uint64_t>())
		.AddAttribute("TimelyTHigh",
				"THigh of TIMELY (ns)",
				UintegerValue(500000),
				MakeUintegerAccessor(&RdmaHw::m_tmly_THigh),
				MakeUintegerChecker<uint64_t>())
		.AddAttribute("TimelyMinRtt",
				"MinRtt of TIMELY (ns)",
				UintegerValue(20000),
				MakeUintegerAccessor(&RdmaHw::m_tmly_minRtt),
				MakeUintegerChecker<uint64_t>())
		.AddAttribute("DctcpRateAI",
				"DCTCP's Rate increment unit in AI period",
				DataRateValue(DataRate("1000Mb/s")),
				MakeDataRateAccessor(&RdmaHw::m_dctcp_rai),
				MakeDataRateChecker())
		;
	return tid;
}

RdmaHw::RdmaHw(){
	bwId = Simulator::Schedule(MicroSeconds(2000000), &RdmaHw::BW, this);//all applications start at 2s
	bw = 0;
	tp = 0;
	ctrl = 0;
	totalQueuingPackets = 0;
	totalQueuingTimeUs = 0;
}

RdmaHw::~RdmaHw(){
	Simulator::Cancel(bwId);
}

void RdmaHw::SetNode(Ptr<Node> node){
	m_node = node;
}
void RdmaHw::Setup(QpCompleteCallback cb, RcvFinishCallback rcvcb, MsgCompleteCallback mcb){
	for (uint32_t i = 0; i < m_nic.size(); i++){
		Ptr<QbbNetDevice> dev = m_nic[i].dev;
		if (dev == NULL)
			continue;
		// share data with NIC
		dev->m_rdmaEQ->m_qpGrp = m_nic[i].qpGrp;
		// setup callback
		dev->m_rdmaReceiveCb = MakeCallback(&RdmaHw::Receive, this);
		dev->m_rdmaLinkDownCb = MakeCallback(&RdmaHw::SetLinkDown, this);
		dev->m_rdmaPktSent = MakeCallback(&RdmaHw::PktSent, this);
		// config NIC
		dev->m_rdmaEQ->m_mtu = m_mtu;
		dev->m_rdmaEQ->m_rdmaGetNxtPkt = MakeCallback(&RdmaHw::GetNxtPacket, this);
	}
	// setup qp complete callback
	m_qpCompleteCallback = cb;
	m_rcvFinishCallback = rcvcb;
	m_msgCompleteCallback = mcb;
}

uint32_t RdmaHw::GetNicIdxOfQp(Ptr<RdmaQueuePair> qp){
	auto &v = m_rtTable[qp->dip.Get()];
	if (v.size() > 0){
		return v[qp->GetHash() % v.size()];
	}else{
		NS_ASSERT_MSG(false, "We assume at least one NIC is alive");
	}
}
uint64_t RdmaHw::GetQpKey(uint16_t sport, uint16_t pg){
	return ((uint64_t)sport << 16) | (uint64_t)pg;
}

Ptr<RdmaQueuePair> RdmaHw::GetQp(uint16_t sport, uint16_t pg){
	uint64_t key = GetQpKey(sport, pg);
	auto it = m_qpMap.find(key);
	if (it != m_qpMap.end())
		return it->second;
	return NULL;
}

Ptr<RdmaQueuePair> RdmaHw::GetQp(uint32_t qpid){
	auto it = m_qpMap.find(qpid);
	if (it != m_qpMap.end())
		return it->second;
	return NULL;
}

void RdmaHw::AddQueuePair(uint64_t size, uint16_t pg, Ipv4Address sip, Ipv4Address dip, uint16_t sport, uint16_t dport, uint32_t win, uint64_t baseRtt, uint32_t qpid, uint32_t msgSeq, uint32_t src, uint32_t dst, uint32_t flow_id, bool isTestFlow){

	m_win_initial = win;
	m_baseRtt_initial = baseRtt;

	if (!Settings::qp_mode){
		// create qp
		Ptr<RdmaQueuePair> qp = CreateObject<RdmaQueuePair>(pg, sip, dip, sport, dport);
		qp->SetSize(size);
		qp->SetWin(win);
		qp->SetBaseRtt(baseRtt);
		qp->SetVarWin(m_var_win);
		qp->SetQPId(qpid);
		qp->SetMSGSeq(msgSeq);
		qp->SetSrc(src);
		qp->SetDst(dst);
		qp->SetFlowId(flow_id);
		qp->SetTestFlow(isTestFlow);

		// add qp
		uint32_t nic_idx = GetNicIdxOfQp(qp);
		m_nic[nic_idx].qpGrp->AddQp(qp);
		uint64_t key = GetQpKey(sport, pg);
		m_qpMap[key] = qp;

		/*[hyx]*/
		// 添加qpid，只有在使用rnic cache时才会添加
		if(Settings::use_rnic_cache){
			m_nic[nic_idx].dev->qpccache.insert(qpid);
		}

		// set init variables
		DataRate m_bps = m_nic[nic_idx].dev->GetDataRate();
		qp->m_rate = m_bps;
		qp->m_max_rate = m_bps;
		qp->PrintRate();
		if (m_cc_mode == 1){
			qp->mlx.m_targetRate = m_bps;
		}else if (m_cc_mode == 3){
			qp->hp.m_curRate = m_bps;
			if (m_multipleRate){
				for (uint32_t i = 0; i < IntHeader::maxHop; i++)
					qp->hp.hopState[i].Rc = m_bps;
			}
		}else if (m_cc_mode == 7){
			qp->tmly.m_curRate = m_bps;
		}
		qp->SetDevDequeueCallback(MakeCallback(&QbbNetDevice::TriggerTransmit, m_nic[nic_idx].dev));
		// Notify Nic
		m_nic[nic_idx].dev->NewQp(qp);
	}else{
		// create message
		Ptr<RdmaOperation> msg = Create<RdmaOperation>(pg, sip, dip, sport, dport);
		msg->SetSrc(src);
		msg->SetDst(dst);
		msg->SetMSGSeq(msgSeq);
		msg->SetQPId(qpid);
		msg->SetSize(size);
		msg->SetFlowId(flow_id);
		msg->SetTestFlow(isTestFlow);

		// check qp and add message into qp
		uint64_t key = qpid;	// use qpid as key
		if (m_qpMap.find(key) == m_qpMap.end()) {
			Ptr<RdmaQueuePair> qp = CreateObject<RdmaQueuePair>(pg, sip, dip, sport, dport);
			qp->SetWin(win);
			qp->SetBaseRtt(baseRtt);
			qp->SetVarWin(m_var_win);
			qp->SetQPId(qpid);
			qp->SetSrc(src);
			qp->SetDst(dst);
			qp->SetTestFlow(isTestFlow);
			qp->AddRdmaOperation(msg);

			uint32_t nic_idx = GetNicIdxOfQp(qp);
			m_nic[nic_idx].qpGrp->AddQp(qp);
			m_qpMap[key] = qp;

			// set init variables
			DataRate m_bps = m_nic[nic_idx].dev->GetDataRate();
			qp->m_rate = m_bps;
			qp->m_max_rate = m_bps;
			qp->PrintRate();
			if (m_cc_mode == 1){
				qp->mlx.m_targetRate = m_bps;
			}else if (m_cc_mode == 3){
				qp->hp.m_curRate = m_bps;
				if (m_multipleRate){
					for (uint32_t i = 0; i < IntHeader::maxHop; i++)
						qp->hp.hopState[i].Rc = m_bps;
				}
			}else if (m_cc_mode == 7){
				qp->tmly.m_curRate = m_bps;
			}

			qp->SetDevDequeueCallback(MakeCallback(&QbbNetDevice::TriggerTransmit, m_nic[nic_idx].dev));
			// Notify Nic
			m_nic[nic_idx].dev->NewQp(qp);

		}else{
			m_qpMap[key]->AddRdmaOperation(msg);
			// Notify Nic
			if (m_qpMap[key]->GetMsgNumber() == 1){
				uint32_t nic_idx = GetNicIdxOfQp(m_qpMap[key]);
				m_nic[nic_idx].dev->NewQp(m_qpMap[key]);
			}
		}
	}
}

Ptr<RdmaRxQueuePair> RdmaHw::GetRxQp(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, uint32_t flowId, bool create){
	uint64_t key = ((uint64_t)dip << 32) | ((uint64_t)pg << 16) | (uint64_t)dport;
	auto it = m_rxQpMap.find(key);
	if (it != m_rxQpMap.end())
		return it->second;
	if (create){
		// create new rx qp
		Ptr<RdmaRxQueuePair> q = CreateObject<RdmaRxQueuePair>();
		// init the qp
		q->sip = sip;
		q->dip = dip;
		q->sport = sport;
		q->dport = dport;
		q->m_ecn_source.qIndex = pg;
		q->m_flowId = flowId;
		// store in map
		m_rxQpMap[key] = q;
		return q;
	}
	return NULL;
}

uint32_t RdmaHw::GetNicIdxOfRxQp(Ptr<RdmaRxQueuePair> q){
	auto &v = m_rtTable[q->dip];
	if (v.size() > 0){
		return v[q->GetHash() % v.size()];
	}else{
		NS_ASSERT_MSG(false, "We assume at least one NIC is alive");
	}
}

int RdmaHw::ReceiveUdp(Ptr<Packet> p, CustomHeader &ch){
	uint8_t ecnbits = ch.GetIpv4EcnBits();

	uint32_t payload_size = p->GetSize() - ch.GetSerializedSize();

	tp += payload_size;
	bw += p->GetSize();

	FlowTag ftag;
	p->PeekPacketTag(ftag);

	Ptr<RdmaRxQueuePair> rxQp = GetRxQp(ch.dip, ch.sip, ch.udp.dport, ch.udp.sport, ch.udp.pg, ftag.getIndex(), true);
	if (ecnbits != 0){
		rxQp->m_ecn_source.ecnbits |= ecnbits;
		rxQp->m_ecn_source.qfb++;
	}
	rxQp->m_ecn_source.total++;
	rxQp->m_milestone_rx = m_ack_interval;

	// update queuing time
	QueueingTag q;
	if(p->RemovePacketTag(q)){
		uint64_t max_hop = q.GetActiveHop();
		for (uint64_t i = 0; i < max_hop; ++i){
			rxQp->m_queuingTime[i].push_back(q.GetHopQueuingTime(i));
		}
	}

	QPTag qptag;
	p->PeekPacketTag(qptag);

	int x = ReceiverCheckSeq(ch.udp.seq, rxQp, payload_size);

	bool receivedAll = false;
	rxQp->m_received_psn.insert(ch.udp.seq);
	LastPacketTag lastTag;
	if (p->PeekPacketTag(lastTag)){
		rxQp->m_received_last_psn_packet = true;
	}
	receivedAll = rxQp->ReceivedAll(Settings::packet_payload);

	if (Settings::IsPacketLevelRouting()){
		/**
		 * when use packet-level routing
		 * --> unordered packets are normal
		 * --> so, always send ACK
		 */
		x = 1;
	}
	if (x == 1 || x == 2){ //generate ACK or NACK

		qbbHeader seqh;
		seqh.SetSeq(rxQp->ReceiverNextExpectedSeq);
		if (Settings::IsPacketLevelRouting()){
			seqh.SetSeq(ch.udp.seq);
		}
		seqh.SetPG(ch.udp.pg);
		seqh.SetSport(ch.udp.dport);
		seqh.SetDport(ch.udp.sport);
		seqh.SetIntHeader(ch.udp.ih);
		if (ecnbits)
			seqh.SetCnp();

		//Ptr<Packet> newp = Create<Packet>(std::max(60-14-20-(int)seqh.GetSerializedSize(), 0));// at least 60
		Ptr<Packet> newp = Create<Packet>(std::max(84-38-20-(int)seqh.GetSerializedSize(), 0));// at least 84, modified by lkx
		newp->AddHeader(seqh);

		//20 bytes
		Ipv4Header head;	// Prepare IPv4 header
		head.SetDestination(Ipv4Address(ch.sip));
		head.SetSource(Ipv4Address(ch.dip));
		head.SetProtocol(x == 1 ? 0xFC : 0xFD); //ack=0xFC nack=0xFD
		head.SetTtl(64);
		head.SetPayloadSize(newp->GetSize());
		head.SetIdentification(rxQp->m_ipid++);

		newp->AddHeader(head);

		//ppp: 14 + 24
		AddHeader(newp, 0x800);	// Attach PPP header

		if (Settings::qp_mode){
			QPTag qptag;
			p->PeekPacketTag(qptag);
			newp->AddPacketTag(qptag);
		}

		/**
		 * ACK will piggy back the switchACK info for restore VOQ's window
		 */
		if (Settings::use_floodgate && Settings::switch_ack_mode == Settings::HOST_PER_PACKET_ACK){
			SwitchACKTag acktag;
			acktag.setAckedSize(p->GetSize());
			newp->AddPacketTag(acktag);
		}

		/**
		 * let ACK go symmetric path if has set it.
		 */
		if (Settings::symmetic_routing_mode == Settings::SYMMETRIC_RECEIVER){
			RecordRoutingTag routingTag;
			if (p->PeekPacketTag(routingTag)) {
				SymmetricRoutingTag symmetricTag;
				symmetricTag.resetIndex();
				for(uint32_t i = MAX_HOP-1; i != (uint32_t)-1; --i){
					uint32_t curr = routingTag.getReceiverLeafIngress(i);
					if (curr == 0) continue;
					symmetricTag.setReceiverLeafIngress(curr);
				}
				symmetricTag.resetIndex();
				newp->AddPacketTag(symmetricTag);
			}
		}

		if (receivedAll){
			LastPacketTag lastTag;
			newp->AddPacketTag(lastTag);
			RcvFinish(rxQp);
		}

		// send
		uint32_t nic_idx = GetNicIdxOfRxQp(rxQp);
		m_nic[nic_idx].dev->RdmaEnqueueHighPrioQ(newp);
		m_nic[nic_idx].dev->TriggerTransmit();
	}
	return 0;
}

int RdmaHw::ReceiveCnp(Ptr<Packet> p, CustomHeader &ch){
	// QCN on NIC
	// This is a Congestion signal
	// Then, extract data from the congestion packet.
	// We assume, without verify, the packet is destinated to me
	uint32_t qIndex = ch.cnp.qIndex;
	if (qIndex == 1){		//DCTCP
		std::cout << "TCP--ignore\n";
		return 0;
	}
	uint16_t udpport = ch.cnp.fid; // corresponds to the sport
	uint8_t ecnbits = ch.cnp.ecnBits;
	uint16_t qfb = ch.cnp.qfb;
	uint16_t total = ch.cnp.total;

	uint32_t i;

	// get qp
	Ptr<RdmaQueuePair> qp = NULL;
	if (Settings::qp_mode){
		QPTag qptag;
		p->PeekPacketTag(qptag);
		qp = GetQp(qptag.GetQPID());
	}else{
		qp = GetQp(udpport, qIndex);
	}

	if (qp == NULL)
		std::cout << "ERROR: QCN NIC cannot find the flow\n";
	// get nic
	uint32_t nic_idx = GetNicIdxOfQp(qp);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;

	if (qp->m_rate == 0)			//lazy initialization	
	{
		qp->m_rate = dev->GetDataRate();
		if (m_cc_mode == 1){
			qp->mlx.m_targetRate = dev->GetDataRate();
		}else if (m_cc_mode == 3){
			qp->hp.m_curRate = dev->GetDataRate();
			if (m_multipleRate){
				for (uint32_t i = 0; i < IntHeader::maxHop; i++)
					qp->hp.hopState[i].Rc = dev->GetDataRate();
			}
		}else if (m_cc_mode == 7){
			qp->tmly.m_curRate = dev->GetDataRate();
		}
	}
	qp->PrintRate();
	return 0;
}

int RdmaHw::ReceiveAck(Ptr<Packet> p, CustomHeader &ch){

	uint16_t qIndex = ch.ack.pg;
	uint16_t port = ch.ack.dport;
	uint32_t seq = ch.ack.seq;
	uint8_t cnp = (ch.ack.flags >> qbbHeader::FLAG_CNP) & 1;
	int i;
	Ptr<RdmaQueuePair> qp = NULL;
	uint32_t msg_seq = 0;
	if (Settings::qp_mode){
		QPTag qptag;
		p->PeekPacketTag(qptag);
		qp = GetQp(qptag.GetQPID());
		msg_seq = qptag.GetMsgSeq();
	}else{
		qp = GetQp(port, qIndex);
	}

	if (qp == NULL){
		std::cerr << "ERROR: " << "node:" << m_node->GetId() << ' ' << (ch.l3Prot == 0xFC ? "ACK" : "NACK") << " NIC cannot find the flow\n";
		return 0;
	}

	/*[hyx]*/
	// 收到ack，数据传输完成，wqe数量减一
	bool delay = false;
	if(Settings::use_rnic_cache){
		// for weir
		/*[Delay Monitor]*/
		if(Settings::use_weir){  // weir mode after receiving ack
			uint64_t rtt = Simulator::Now().GetTimeStep() - ch.ack.ih.ts;
			qp->pre_rtt = qp->cur_rtt;
			qp->cur_rtt = rtt;
		}

		uint32_t nic_idx = GetNicIdxOfQp(qp);
		// mtt mpt nothing to do
		// wqe
	if(!Settings::use_weir){
		m_nic[nic_idx].dev->wqecache.decrement();
	}
		// qpc
		// for DCT
		if(!Settings::use_dct){
			// DCT 不会造成 qpc 的丢失，因此只有非 DCT 的方案才需要考虑 qpc cache 是否在 list 之中的情况
			if(!m_nic[nic_idx].dev->qpccache.contains(qp->m_qpid)){
				m_nic[nic_idx].dev->qpccache.insert(qp->m_qpid);
				delay = true;
			}
		}
	}

	bool isLastACK = false;
	LastPacketTag lastTag;
	isLastACK = p->PeekPacketTag(lastTag);

	uint32_t nic_idx = GetNicIdxOfQp(qp);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;
	if (m_ack_interval == 0)
		std::cout << "ERROR: shouldn't receive ack\n";
	else {
		if (!Settings::qp_mode){
			if (!m_backto0){
				qp->Acknowledge(seq, isLastACK);
			}else {
				uint32_t goback_seq = seq / m_chunk * m_chunk;
				qp->Acknowledge(goback_seq, isLastACK);
			}
			if (qp->IsFinished()){
				QpComplete(qp);
			}
		}else{
			if (msg_seq == qp->m_msgSeq){
				if (!m_backto0){
					qp->Acknowledge(seq, isLastACK);
				}else {
					uint32_t goback_seq = seq / m_chunk * m_chunk;
					qp->Acknowledge(goback_seq, isLastACK);
				}
			}else{
				if (!m_backto0){
					qp->Acknowledge(seq, msg_seq, isLastACK);
				}else {
					uint32_t goback_seq = seq / m_chunk * m_chunk;
					qp->Acknowledge(goback_seq, msg_seq, isLastACK);
				}
			}

			Ptr<RdmaOperation> msg = qp->CheckFinish(msg_seq, seq);
			if (!!msg){
				MsgComplete(qp, msg);
			}
		}
	}
	if (ch.l3Prot == 0xFD && Settings::nack_reaction){ // NACK
		if (Settings::qp_mode){
			if (msg_seq == qp->m_msgSeq)
				RecoverQueue(qp);
			else {
				qp->RecoverMsg(qp->m_msgSeq);
			}
		}else{
			RecoverQueue(qp);
		}
	}

	// for timeout retransmission
	if (Settings::rto_us != 0){
		if (!Settings::qp_mode && qp->GetBytesLeft() == 0 && qp->GetOnTheFly() > 0){
			qp->ResetRTOEvent();
		}else if (Settings::qp_mode){
			qp->ResetMSGRTOTime(msg_seq);
		}
	}

	// handle cnp
	if (cnp){
		if (m_cc_mode == 1){ // mlx version
			cnp_received_mlx(qp);
		} 
	}

	if (m_cc_mode == 3){
		HandleAckHp(qp, p, ch);
	}else if (m_cc_mode == 7){
		HandleAckTimely(qp, p, ch);
	}else if (m_cc_mode == 8){
		if (Settings::IsPacketLevelRouting())
			HandleDisOrderedAckDctcp(qp, p, ch);
		else
			HandleAckDctcp(qp, p, ch);
	}
	// ACK may advance the on-the-fly window, allowing more packets to send
	/*[hyx]*/
	// xrc 在共享接收队列的建模不在此处完成，否则会产生过多的修改，直接放在 TriggerTransmitWithDelay 以及 TriggerTransmit 中完成 （qbb-net-device.cc file）
	// 此处只需要判断是否需要delay即可
	if(Settings::use_rnic_cache){
		if(delay){
			dev->TriggerTransmitWithDelay();
		}else{
			dev->TriggerTransmit();
		}
		return 0;
	}
	dev->TriggerTransmit();
	return 0;
}

int RdmaHw::Receive(Ptr<Packet> p, CustomHeader &ch){
	// for debug
	QPTag qptag;
	p->PeekPacketTag(qptag);
	uint32_t qpId = qptag.GetQPID();
	uint32_t msgSeq = qptag.GetMsgSeq();
	qpId++;
	msgSeq++;

	if (ch.l3Prot == 0x11){ // UDP
		ReceiveUdp(p, ch);

		//for analysis, added by lkx
		QueueingTag q;
		if(p->RemovePacketTag(q)){
			++totalQueuingPackets;
			totalQueuingTimeUs += q.GetQueueingTimeUs();
		}
	}else if (ch.l3Prot == 0xFF){ // CNP
		ctrl += p->GetSize();
		ReceiveCnp(p, ch);
	}else if (ch.l3Prot == 0xFD){ // NACK
		ctrl += p->GetSize();
		ReceiveAck(p, ch);
	}else if (ch.l3Prot == 0xFC){ // ACK
		ctrl += p->GetSize();
		ReceiveAck(p, ch);
	}
	return 0;
}

int RdmaHw::ReceiverCheckSeq(uint32_t seq, Ptr<RdmaRxQueuePair> q, uint32_t size){
	uint32_t expected = q->ReceiverNextExpectedSeq;
	if (seq == expected){
		q->ReceiverNextExpectedSeq = expected + size;
		if (q->ReceiverNextExpectedSeq >= q->m_milestone_rx){
			q->m_milestone_rx += m_ack_interval;
			return 1; //Generate ACK
		}else if (q->ReceiverNextExpectedSeq % m_chunk == 0){
			return 1;
		}else {
			return 5;
		}
	} else if (seq > expected) {
		// Generate NACK
		if (Simulator::Now() >= q->m_nackTimer || q->m_lastNACK != expected){
			q->m_nackTimer = Simulator::Now() + MicroSeconds(m_nack_interval);
			q->m_lastNACK = expected;
			if (m_backto0){
				q->ReceiverNextExpectedSeq = q->ReceiverNextExpectedSeq / m_chunk*m_chunk;
			}
			return 2;
		}else
			return 4;
	}else {
		// Duplicate.
		return 3;
	}
}
void RdmaHw::AddHeader (Ptr<Packet> p, uint16_t protocolNumber){
	PppHeader ppp;
	ppp.SetProtocol (EtherToPpp (protocolNumber));
	p->AddHeader (ppp);
}
uint16_t RdmaHw::EtherToPpp (uint16_t proto){
	switch(proto){
		case 0x0800: return 0x0021;   //IPv4
		case 0x86DD: return 0x0057;   //IPv6
		default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
	}
	return 0;
}

void RdmaHw::RecoverQueue(Ptr<RdmaQueuePair> qp){
	qp->snd_nxt = qp->snd_una;
}

void RdmaHw::ResetQP(Ptr<RdmaQueuePair> qp){
	qp->SetWin(m_win_initial);
	qp->SetBaseRtt(m_baseRtt_initial);
	qp->SetVarWin(m_var_win);
	uint32_t nic_idx = GetNicIdxOfQp(qp);
	DataRate m_bps = m_nic[nic_idx].dev->GetDataRate();
	qp->m_rate = m_bps;
	qp->m_max_rate = m_bps;
	if (m_cc_mode == 1){
		qp->mlx.m_targetRate = m_bps;
	}else if (m_cc_mode == 3){
		qp->hp.m_curRate = m_bps;
		if (m_multipleRate){
			for (uint32_t i = 0; i < IntHeader::maxHop; i++)
				qp->hp.hopState[i].Rc = m_bps;
		}
	}else if (m_cc_mode == 7){
		qp->tmly.m_curRate = m_bps;
	}
	qp->PrintRate();
}

void RdmaHw::MsgComplete(Ptr<RdmaQueuePair> qp, Ptr<RdmaOperation> msg){
	m_msgCompleteCallback(msg);
	if (!qp->RemoveRdmaOperation(msg->m_msgSeq)){
		std::cout << "ERROR: remove message\n";
	}
}

void RdmaHw::RcvFinish(Ptr<RdmaRxQueuePair> qp){
	NS_ASSERT(!m_rcvFinishCallback.IsNull());
	m_rcvFinishCallback(qp);
}

void RdmaHw::QpComplete(Ptr<RdmaQueuePair> qp){
	NS_ASSERT(!m_qpCompleteCallback.IsNull());
	if (m_cc_mode == 1){
		Simulator::Cancel(qp->mlx.m_eventUpdateAlpha);
		Simulator::Cancel(qp->mlx.m_eventDecreaseRate);
		Simulator::Cancel(qp->mlx.m_rpTimer);
	}
	Simulator::Cancel(qp->m_eventTimeoutRetransmission);

	m_qpCompleteCallback(qp);
}

void RdmaHw::SetLinkDown(Ptr<QbbNetDevice> dev){
	printf("RdmaHw: node:%u a link down\n", m_node->GetId());
}

void RdmaHw::AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx){
	uint32_t dip = dstAddr.Get();
	m_rtTable[dip].push_back(intf_idx);
}

void RdmaHw::ClearTable(){
	m_rtTable.clear();
}

void RdmaHw::RedistributeQp(){
	// clear old qpGrp
	for (uint32_t i = 0; i < m_nic.size(); i++){
		if (m_nic[i].dev == NULL)
			continue;
		m_nic[i].qpGrp->Clear();
	}

	// redistribute qp
	for (auto &it : m_qpMap){
		Ptr<RdmaQueuePair> qp = it.second;
		uint32_t nic_idx = GetNicIdxOfQp(qp);
		m_nic[nic_idx].qpGrp->AddQp(qp);
		// Notify Nic
		m_nic[nic_idx].dev->ReassignedQp(qp);
	}
}

Ptr<Packet> RdmaHw::GetNxtPacket(Ptr<RdmaQueuePair> qp){

	uint32_t seq = qp->snd_nxt;
	if (Settings::IsPacketLevelRouting()){
		if (seq >= qp->m_size && qp->timeouted && qp->m_timeout_psn.size() > 0){
			/**
			 * Under packet-level-routing, don't use go-back-N.
			 * Flow has sent out all packets at once (qp->snd_nxt >= qp->m_size),
			 * however, till timeout triggered(qp->timeouted), there are some in-flight data packet(m_timeout_psn).
			 * These (ACKs of)packets may dropped, so retransmit these packets.
			 */
			seq = *(qp->m_timeout_psn.begin());
			qp->m_timeout_psn.erase(qp->m_timeout_psn.begin());
			assert(seq < qp->m_size);
		}else{
			qp->m_sent_psn.insert(seq);
		}
	}

	uint32_t payload_size = m_mtu;
	bool is_last = false;
	if (seq + m_mtu >= qp->m_size){
		is_last = true;
		payload_size = qp->m_size - seq;
	}

	/*[hyx]*/
	// 连接可扩展问题主要是出现在发送数据，因此应该在发送数据这部分引入额外的带宽损失，接收数据的带宽与发送数据的带宽无关
	// uint64_t extra_pcie_bw = 0;  // Gbps
	if(Settings::use_rnic_cache){
		uint64_t extra_pcie_bw = 0;
		// mtt mpt
		int mtt_mpt_probability = generateMttMptProbability();
		if(mtt_mpt_probability == 0){
			extra_pcie_bw += 4;
			// random pcie bw
			uint64_t random_pcie_bw = GetRandomPCIeBW();
			extra_pcie_bw += random_pcie_bw;
		}else{
			extra_pcie_bw += 0;
		}
		// wqe （首先需要获取 nic idx ）
		uint32_t nic_idx = GetNicIdxOfQp(qp);
	if(!Settings::use_weir){
		if(m_nic[nic_idx].dev->wqecache.contains()){
			extra_pcie_bw += 0;
		}else{
			extra_pcie_bw += 8;
			// random pcie bw
			uint64_t random_pcie_bw = GetRandomPCIeBW();
			extra_pcie_bw += random_pcie_bw;
			// extra pcie bw of mtt mpt for wqe cache miss
			int mtt_mpt_probability1 = generateMttMptProbability1();
			if(mtt_mpt_probability1 == 0){
				extra_pcie_bw += 4;
			}else{
				extra_pcie_bw += 0;
			}
		}
	}
		// qpc
		// for DCT
		if(!Settings::use_dct){
			// DCT 不会造成 qpc 的丢失，因此只有非 DCT 的方案才需要考虑 qpc cache 是否在 list 之中的情况
			if(m_nic[nic_idx].dev->qpccache.contains(qp->m_qpid)){
				extra_pcie_bw += 0;
			}else{
				extra_pcie_bw += 6;
				// random pcie bw
				uint64_t random_pcie_bw = GetRandomPCIeBW();
				extra_pcie_bw += random_pcie_bw;
				// extra pcie bw of mtt mpt for qpc cache miss
				int mtt_mpt_probability2 = generateMttMptProbability2();
				if(mtt_mpt_probability2 == 0){
					extra_pcie_bw += 4;
				}else{
					extra_pcie_bw += 0;
				}
			}
		}

		// 计算并更新新的速率
		uint64_t qp_new_rate = qp->m_rate.GetBitRate() - extra_pcie_bw * 1024 * 1024 * 1024;
		DataRate new_rate = DataRate(qp_new_rate);  // bps
		// 改变发送数据的速率
		ChangeRate(qp, new_rate);

	}

	Ptr<Packet> p = Create<Packet> (payload_size);
	// add SeqTsHeader
	SeqTsHeader seqTs;
	seqTs.SetSeq (seq);
	seqTs.SetPG (qp->m_pg);
	p->AddHeader (seqTs);
	// add udp header
	UdpHeader udpHeader;
	udpHeader.SetDestinationPort (qp->dport);
	udpHeader.SetSourcePort (qp->sport);
	p->AddHeader (udpHeader);
	// add ipv4 header
	Ipv4Header ipHeader;
	ipHeader.SetSource (qp->sip);
	ipHeader.SetDestination (qp->dip);
	ipHeader.SetProtocol (0x11);
	ipHeader.SetPayloadSize (p->GetSize());
	ipHeader.SetTtl (64);
	ipHeader.SetTos (0);
	ipHeader.SetIdentification (qp->m_ipid);
	p->AddHeader(ipHeader);
	// add ppp header
	PppHeader ppp;
	ppp.SetProtocol (0x0021); // EtherToPpp(0x800), see point-to-point-net-device.cc
	p->AddHeader (ppp);

	//add queueing tag
	QueueingTag tag;
	p->AddPacketTag(tag);

	FlowTag ftag;
	ftag.setIndex(qp->m_flow_id);
	p->AddPacketTag(ftag);

	if (Settings::qp_mode){
		QPTag qptag;
		uint32_t qpId = qp->m_qpid;
		uint32_t msgSeq = qp->m_msgSeq;
		qptag.SetQPID(qpId);
		qptag.SetMsgSeq(msgSeq);

		p->AddPacketTag(qptag);
	}

	if (is_last){
		LastPacketTag lastTag;
		p->AddPacketTag(lastTag);
	}

	// update state
	if (seq == qp->snd_nxt) qp->snd_nxt += payload_size;
	qp->m_ipid++;

	// for timeout retransmission
	if (Settings::rto_us != 0){
		if (!Settings::qp_mode && qp->GetBytesLeft() == 0 && qp->GetOnTheFly() > 0){
			qp->ResetRTOEvent();
		}else if (Settings::qp_mode){
			qp->ResetMSGRTOTime(qp->m_msgSeq);
		}
	}

	/*
	 * If sent all packets, move msg into unfinished queue
	 * --> When !Settings::IsPacketLevelRouting(), only check qp->snd_nxt >= qp->m_size
	 * --> else, check qp->snd_nxt >= qp->m_size && (!qp->timeouted || qp->m_timeout_psn.empty())
	 */
	if (Settings::qp_mode){
		if (qp->GetBytesLeft() == 0){
			qp->MoveRdmaOperationToUnfinished();
			if (Settings::reset_qp_rate)
				ResetQP(qp);
		}
	}
	return p;
}

void RdmaHw::PktSent(Ptr<RdmaQueuePair> qp, Ptr<Packet> pkt, Time interframeGap){
	qp->lastPktSize = pkt->GetSize();
	UpdateNextAvail(qp, interframeGap, pkt->GetSize());
}

void RdmaHw::UpdateNextAvail(Ptr<RdmaQueuePair> qp, Time interframeGap, uint32_t pkt_size){
	Time sendingTime;
	if (m_rateBound)
		sendingTime = interframeGap + Seconds(qp->m_rate.CalculateTxTime(pkt_size));
	else
		sendingTime = interframeGap + Seconds(qp->m_max_rate.CalculateTxTime(pkt_size));
	qp->m_nextAvail = Simulator::Now() + sendingTime;
}

void RdmaHw::ChangeRate(Ptr<RdmaQueuePair> qp, DataRate new_rate){
	#if 1
	Time sendingTime = Seconds(qp->m_rate.CalculateTxTime(qp->lastPktSize));
	Time new_sendintTime = Seconds(new_rate.CalculateTxTime(qp->lastPktSize));
	qp->m_nextAvail = qp->m_nextAvail + new_sendintTime - sendingTime;
	// update nic's next avail event
	uint32_t nic_idx = GetNicIdxOfQp(qp);
	m_nic[nic_idx].dev->UpdateNextAvail(qp->m_nextAvail);
	#endif

	// change to new rate
	qp->m_rate = new_rate;

	qp->PrintRate();
}

//added
void RdmaHw::BW(){
	double total = Settings::host_bw_Bps * Settings::bw_interval/1000000.0;//the unit of BW_INTERVAL is us
	uint32_t hostId = m_node->GetId();
	Settings::bwList[hostId].push_back(bw / total);
	Settings::tpList[hostId].push_back(tp / total);
	Settings::ctrlList[hostId].push_back(ctrl / total);
	if(totalQueuingPackets == 0)//do not receive a packet at this time
		Settings::QDelayList[hostId].push_back(0);
	else
		Settings::QDelayList[hostId].push_back(totalQueuingTimeUs/totalQueuingPackets);

	if (Settings::bw_out.is_open())
		Settings::bw_out << hostId << " " << Simulator::Now() << " " << ctrl+bw << " " << bw << " " << tp << std::endl;

	bw = 0;
	tp = 0;
	ctrl = 0;
	totalQueuingPackets = 0;
	totalQueuingTimeUs = 0;
	bwId = Simulator::Schedule(MicroSeconds(Settings::bw_interval), &RdmaHw::BW, this);
}

#define PRINT_LOG 0
/******************************
 * Mellanox's version of DCQCN
 *****************************/
void RdmaHw::UpdateAlphaMlx(Ptr<RdmaQueuePair> q){
	#if PRINT_LOG
	//std::cout << Simulator::Now() << " alpha update:" << m_node->GetId() << ' ' << q->mlx.m_alpha << ' ' << (int)q->mlx.m_alpha_cnp_arrived << '\n';
	//printf("%lu alpha update: %08x %08x %u %u %.6lf->", Simulator::Now().GetTimeStep(), q->sip.Get(), q->dip.Get(), q->sport, q->dport, q->mlx.m_alpha);
	#endif
	if (q->mlx.m_alpha_cnp_arrived){
		q->mlx.m_alpha = (1 - m_g)*q->mlx.m_alpha + m_g; 	//binary feedback
	}else {
		q->mlx.m_alpha = (1 - m_g)*q->mlx.m_alpha; 	//binary feedback
	}
	#if PRINT_LOG
	//printf("%.6lf\n", q->mlx.m_alpha);
	#endif
	q->mlx.m_alpha_cnp_arrived = false; // clear the CNP_arrived bit
	ScheduleUpdateAlphaMlx(q);
}
void RdmaHw::ScheduleUpdateAlphaMlx(Ptr<RdmaQueuePair> q){
	q->mlx.m_eventUpdateAlpha = Simulator::Schedule(MicroSeconds(m_alpha_resume_interval), &RdmaHw::UpdateAlphaMlx, this, q);
}

void RdmaHw::cnp_received_mlx(Ptr<RdmaQueuePair> q){
	q->mlx.m_alpha_cnp_arrived = true; // set CNP_arrived bit for alpha update
	q->mlx.m_decrease_cnp_arrived = true; // set CNP_arrived bit for rate decrease
	if (q->mlx.m_first_cnp){
		// init alpha
		q->mlx.m_alpha = 1;
		q->mlx.m_alpha_cnp_arrived = false;
		// schedule alpha update
		ScheduleUpdateAlphaMlx(q);
		// schedule rate decrease
		ScheduleDecreaseRateMlx(q, 1); // add 1 ns to make sure rate decrease is after alpha update
		// set rate on first CNP
		q->mlx.m_targetRate = q->m_rate = m_rateOnFirstCNP * q->m_rate;
		q->PrintRate();
		q->mlx.m_first_cnp = false;
	}
}

void RdmaHw::CheckRateDecreaseMlx(Ptr<RdmaQueuePair> q){
	ScheduleDecreaseRateMlx(q, 0);
	if (q->mlx.m_decrease_cnp_arrived){
		#if PRINT_LOG
		printf("%lu rate dec: %08x %08x %u %u (%0.3lf %.3lf)->", Simulator::Now().GetTimeStep(), q->sip.Get(), q->dip.Get(), q->sport, q->dport, q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
		#endif
		bool clamp = true;
		if (!m_EcnClampTgtRate){
			if (q->mlx.m_rpTimeStage == 0)
				clamp = false;
		}
		if (clamp)
			q->mlx.m_targetRate = q->m_rate;
		q->m_rate = std::max(m_minRate, q->m_rate * (1 - q->mlx.m_alpha / 2));
		q->PrintRate();
		// reset rate increase related things
		q->mlx.m_rpTimeStage = 0;
		q->mlx.m_decrease_cnp_arrived = false;
		Simulator::Cancel(q->mlx.m_rpTimer);
		q->mlx.m_rpTimer = Simulator::Schedule(MicroSeconds(m_rpgTimeReset), &RdmaHw::RateIncEventTimerMlx, this, q);
		#if PRINT_LOG
		printf("(%.3lf %.3lf)\n", q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
		#endif
	}
}
void RdmaHw::ScheduleDecreaseRateMlx(Ptr<RdmaQueuePair> q, uint32_t delta){
	q->mlx.m_eventDecreaseRate = Simulator::Schedule(MicroSeconds(m_rateDecreaseInterval) + NanoSeconds(delta), &RdmaHw::CheckRateDecreaseMlx, this, q);
}

void RdmaHw::RateIncEventTimerMlx(Ptr<RdmaQueuePair> q){
	q->mlx.m_rpTimer = Simulator::Schedule(MicroSeconds(m_rpgTimeReset), &RdmaHw::RateIncEventTimerMlx, this, q);
	RateIncEventMlx(q);
	q->mlx.m_rpTimeStage++;
}
void RdmaHw::RateIncEventMlx(Ptr<RdmaQueuePair> q){
	// check which increase phase: fast recovery, active increase, hyper increase
	if (q->mlx.m_rpTimeStage < m_rpgThreshold){ // fast recovery
		FastRecoveryMlx(q);
	}else if (q->mlx.m_rpTimeStage == m_rpgThreshold){ // active increase
		ActiveIncreaseMlx(q);
	}else { // hyper increase
		HyperIncreaseMlx(q);
	}
}

void RdmaHw::FastRecoveryMlx(Ptr<RdmaQueuePair> q){
	#if PRINT_LOG
	printf("%lu fast recovery: %08x %08x %u %u (%0.3lf %.3lf)->", Simulator::Now().GetTimeStep(), q->sip.Get(), q->dip.Get(), q->sport, q->dport, q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
	#endif
	q->m_rate = (q->m_rate / 2) + (q->mlx.m_targetRate / 2);
	#if PRINT_LOG
	printf("(%.3lf %.3lf)\n", q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
	#endif
	q->PrintRate();
}
void RdmaHw::ActiveIncreaseMlx(Ptr<RdmaQueuePair> q){
	#if PRINT_LOG
	printf("%lu active inc: %08x %08x %u %u (%0.3lf %.3lf)->", Simulator::Now().GetTimeStep(), q->sip.Get(), q->dip.Get(), q->sport, q->dport, q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
	#endif
	// get NIC
	uint32_t nic_idx = GetNicIdxOfQp(q);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;
	// increate rate
	q->mlx.m_targetRate += m_rai;
	if (q->mlx.m_targetRate > dev->GetDataRate())
		q->mlx.m_targetRate = dev->GetDataRate();
	q->m_rate = (q->m_rate / 2) + (q->mlx.m_targetRate / 2);
	#if PRINT_LOG
	printf("(%.3lf %.3lf)\n", q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
	#endif
	q->PrintRate();
}
void RdmaHw::HyperIncreaseMlx(Ptr<RdmaQueuePair> q){
	#if PRINT_LOG
	printf("%lu hyper inc: %08x %08x %u %u (%0.3lf %.3lf)->", Simulator::Now().GetTimeStep(), q->sip.Get(), q->dip.Get(), q->sport, q->dport, q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
	#endif
	// get NIC
	uint32_t nic_idx = GetNicIdxOfQp(q);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;
	// increate rate
	q->mlx.m_targetRate += m_rhai;
	if (q->mlx.m_targetRate > dev->GetDataRate())
		q->mlx.m_targetRate = dev->GetDataRate();
	q->m_rate = (q->m_rate / 2) + (q->mlx.m_targetRate / 2);
	#if PRINT_LOG
	printf("(%.3lf %.3lf)\n", q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
	#endif
	q->PrintRate();
}

/***********************
 * High Precision CC
 ***********************/
void RdmaHw::HandleAckHp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
	uint32_t ack_seq = ch.ack.seq;
	// update rate
	if (ack_seq > qp->hp.m_lastUpdateSeq){ // if full RTT feedback is ready, do full update
		UpdateRateHp(qp, p, ch, false);
	}else{ // do fast react
		FastReactHp(qp, p, ch);
	}
}

void RdmaHw::UpdateRateHp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch, bool fast_react){
	uint32_t next_seq = qp->snd_nxt;
	bool print = !fast_react || true;
	if (qp->hp.m_lastUpdateSeq == 0){ // first RTT
		qp->hp.m_lastUpdateSeq = next_seq;
		// store INT
		IntHeader &ih = ch.ack.ih;
		NS_ASSERT(ih.nhop <= IntHeader::maxHop);
		for (uint32_t i = 0; i < ih.nhop; i++)
			qp->hp.hop[i] = ih.hop[i];
		#if PRINT_LOG
		if (print){
			printf("%lu %s %08x %08x %u %u [%u,%u,%u]", Simulator::Now().GetTimeStep(), fast_react? "fast" : "update", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->hp.m_lastUpdateSeq, ch.ack.seq, next_seq);
			for (uint32_t i = 0; i < ih.nhop; i++)
				printf(" %u %lu %lu", ih.hop[i].GetQlen(), ih.hop[i].GetBytes(), ih.hop[i].GetTime());
			printf("\n");
		}
		#endif
	}else {
		// check packet INT
		IntHeader &ih = ch.ack.ih;
		if (ih.nhop <= IntHeader::maxHop){
			double max_c = 0;
			bool inStable = false;
			#if PRINT_LOG
			if (print)
				printf("%lu %s %08x %08x %u %u [%u,%u,%u]", Simulator::Now().GetTimeStep(), fast_react? "fast" : "update", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->hp.m_lastUpdateSeq, ch.ack.seq, next_seq);
			#endif
			// check each hop
			double U = 0;
			uint64_t dt = 0;
			bool updated[IntHeader::maxHop] = {false}, updated_any = false;
			NS_ASSERT(ih.nhop <= IntHeader::maxHop);
			for (uint32_t i = 0; i < ih.nhop; i++){
				if (m_sampleFeedback){
					if (ih.hop[i].GetQlen() == 0 and fast_react)
						continue;
				}
				updated[i] = updated_any = true;
				#if PRINT_LOG
				if (print)
					printf(" %u(%u) %lu(%lu) %lu(%lu)", ih.hop[i].GetQlen(), qp->hp.hop[i].GetQlen(), ih.hop[i].GetBytes(), qp->hp.hop[i].GetBytes(), ih.hop[i].GetTime(), qp->hp.hop[i].GetTime());
				#endif
				uint64_t tau = ih.hop[i].GetTimeDelta(qp->hp.hop[i]);;
				double duration = tau * 1e-9;
				double txRate = (ih.hop[i].GetBytesDelta(qp->hp.hop[i])) * 8 / duration;
				//win is bdp, m_max_rate is the bw of hosts, bdp / bw = T
				double u = txRate / ih.hop[i].GetLineRate() + (double)std::min(ih.hop[i].GetQlen(), qp->hp.hop[i].GetQlen()) * qp->m_max_rate.GetBitRate() / ih.hop[i].GetLineRate() /qp->m_win;
				#if PRINT_LOG
				if (print)
					printf(" %.3lf %.3lf", txRate, u);
				#endif
				if (!m_multipleRate){
					// for aggregate (single R)
					if (u > U){
						U = u;
						dt = tau;
					}
				}else {//lkx: now used
					// for per hop (per hop R)
					if (tau > qp->m_baseRtt)
						tau = qp->m_baseRtt;
					qp->hp.hopState[i].u = (qp->hp.hopState[i].u * (qp->m_baseRtt - tau) + u * tau) / double(qp->m_baseRtt);
				}
				qp->hp.hop[i] = ih.hop[i];//lkx: update hp by using new ih
			}

			DataRate new_rate;
			int32_t new_incStage;
			DataRate new_rate_per_hop[IntHeader::maxHop];
			int32_t new_incStage_per_hop[IntHeader::maxHop];
			if (!m_multipleRate){
				// for aggregate (single R)
				if (updated_any){
					if (dt > qp->m_baseRtt)
						dt = qp->m_baseRtt;
					qp->hp.u = (qp->hp.u * (qp->m_baseRtt - dt) + U * dt) / double(qp->m_baseRtt);
					max_c = qp->hp.u / m_targetUtil;

					if (max_c >= 1 || qp->hp.m_incStage >= m_miThresh){
						new_rate = qp->hp.m_curRate / max_c + m_rai;
						new_incStage = 0;
					}else{
						new_rate = qp->hp.m_curRate + m_rai;
						new_incStage = qp->hp.m_incStage+1;
					}
					if (new_rate < m_minRate)
						new_rate = m_minRate;
					if (new_rate > qp->m_max_rate)
						new_rate = qp->m_max_rate;
					#if PRINT_LOG
					if (print)
						printf(" u=%.6lf U=%.3lf dt=%u max_c=%.3lf", qp->hp.u, U, dt, max_c);
					#endif
					#if PRINT_LOG
					if (print)
						printf(" rate:%.3lf->%.3lf\n", qp->hp.m_curRate.GetBitRate()*1e-9, new_rate.GetBitRate()*1e-9);
					#endif
				}
			}else{
				// for per hop (per hop R)
				new_rate = qp->m_max_rate;
				for (uint32_t i = 0; i < ih.nhop; i++){
					if (updated[i]){
						double c = qp->hp.hopState[i].u / m_targetUtil;
						if (c >= 1 || qp->hp.hopState[i].incStage >= m_miThresh){
							new_rate_per_hop[i] = qp->hp.hopState[i].Rc / c + m_rai;
							new_incStage_per_hop[i] = 0;
						}else{
							new_rate_per_hop[i] = qp->hp.hopState[i].Rc + m_rai;
							new_incStage_per_hop[i] = qp->hp.hopState[i].incStage+1;
						}
						// bound rate
						//lkx: this is not appeared in paper
						if (new_rate_per_hop[i] < m_minRate)
							new_rate_per_hop[i] = m_minRate;
						if (new_rate_per_hop[i] > qp->m_max_rate)
							new_rate_per_hop[i] = qp->m_max_rate;
						// find min new_rate
						if (new_rate_per_hop[i] < new_rate)
							new_rate = new_rate_per_hop[i];
						#if PRINT_LOG
						if (print)
							printf(" [%u]u=%.6lf c=%.3lf", i, qp->hp.hopState[i].u, c);
						#endif
						#if PRINT_LOG
						if (print)
							printf(" %.3lf->%.3lf", qp->hp.hopState[i].Rc.GetBitRate()*1e-9, new_rate.GetBitRate()*1e-9);
						#endif
					}else{
						if (qp->hp.hopState[i].Rc < new_rate)
							new_rate = qp->hp.hopState[i].Rc;
					}
				}
				#if PRINT_LOG
				printf("\n");
				#endif
			}
			if (updated_any)
				ChangeRate(qp, new_rate);//use the min rate among all link
			if (!fast_react){
				if (updated_any){
					qp->hp.m_curRate = new_rate;
					qp->hp.m_incStage = new_incStage;
				}
				if (m_multipleRate){
					// for per hop (per hop R)
					for (uint32_t i = 0; i < ih.nhop; i++){
						if (updated[i]){
							qp->hp.hopState[i].Rc = new_rate_per_hop[i];
							qp->hp.hopState[i].incStage = new_incStage_per_hop[i];
						}
					}
				}
			}
		}
		if (!fast_react){
			if (next_seq > qp->hp.m_lastUpdateSeq)
				qp->hp.m_lastUpdateSeq = next_seq; //+ rand() % 2 * m_mtu;
		}
	}
}

void RdmaHw::FastReactHp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
	if (m_fast_react)
		UpdateRateHp(qp, p, ch, true);
}

/**********************
 * TIMELY
 *********************/
void RdmaHw::HandleAckTimely(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
	uint32_t ack_seq = ch.ack.seq;
	// update rate
	if (ack_seq > qp->tmly.m_lastUpdateSeq){ // if full RTT feedback is ready, do full update
		UpdateRateTimely(qp, p, ch, false);
	}else{ // do fast react
		FastReactTimely(qp, p, ch);
	}
}
void RdmaHw::UpdateRateTimely(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch, bool us){
	uint32_t next_seq = qp->snd_nxt;
	uint64_t rtt = Simulator::Now().GetTimeStep() - ch.ack.ih.ts;
	bool print = !us;
	if (qp->tmly.m_lastUpdateSeq != 0){ // not first RTT
		int64_t new_rtt_diff = (int64_t)rtt - (int64_t)qp->tmly.lastRtt;
		double rtt_diff = (1 - m_tmly_alpha) * qp->tmly.rttDiff + m_tmly_alpha * new_rtt_diff;
		double gradient = rtt_diff / m_tmly_minRtt;
		bool inc = false;
		double c = 0;
		#if PRINT_LOG
		if (print)
			printf("%lu node:%u rtt:%lu rttDiff:%.0lf gradient:%.3lf rate:%.3lf", Simulator::Now().GetTimeStep(), m_node->GetId(), rtt, rtt_diff, gradient, qp->tmly.m_curRate.GetBitRate() * 1e-9);
		#endif
		if (rtt < m_tmly_TLow){
			inc = true;
		}else if (rtt > m_tmly_THigh){
			c = 1 - m_tmly_beta * (1 - (double)m_tmly_THigh / rtt);
			inc = false;
		}else if (gradient <= 0){
			inc = true;
		}else{
			c = 1 - m_tmly_beta * gradient;
			if (c < 0)
				c = 0;
			inc = false;
		}
		if (inc){
			if (qp->tmly.m_incStage < 5){
				qp->m_rate = qp->tmly.m_curRate + m_rai;
			}else{
				qp->m_rate = qp->tmly.m_curRate + m_rhai;
			}
			if (qp->m_rate > qp->m_max_rate)
				qp->m_rate = qp->m_max_rate;
			if (!us){
				qp->tmly.m_curRate = qp->m_rate;
				qp->tmly.m_incStage++;
				qp->tmly.rttDiff = rtt_diff;
			}
		}else{
			qp->m_rate = std::max(m_minRate, qp->tmly.m_curRate * c); 
			if (!us){
				qp->tmly.m_curRate = qp->m_rate;
				qp->tmly.m_incStage = 0;
				qp->tmly.rttDiff = rtt_diff;
			}
		}
		#if PRINT_LOG
		if (print){
			printf(" %c %.3lf\n", inc? '^':'v', qp->m_rate.GetBitRate() * 1e-9);
		}
		#endif
		qp->PrintRate();
	}
	if (!us && next_seq > qp->tmly.m_lastUpdateSeq){
		qp->tmly.m_lastUpdateSeq = next_seq;
		// update
		qp->tmly.lastRtt = rtt;
	}
}
void RdmaHw::FastReactTimely(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
}

/**********************
 * DCTCP
 *********************/
void RdmaHw::HandleAckDctcp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
	uint32_t ack_seq = ch.ack.seq;
	uint8_t cnp = (ch.ack.flags >> qbbHeader::FLAG_CNP) & 1;
	bool new_batch = false;

	// update alpha
	qp->dctcp.m_ecnCnt += (cnp > 0);
	if (ack_seq > qp->dctcp.m_lastUpdateSeq){ // if full RTT feedback is ready, do alpha update
		#if PRINT_LOG
		printf("%lu %s %08x %08x %u %u [%u,%u,%u] %.3lf->", Simulator::Now().GetTimeStep(), "alpha", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->dctcp.m_lastUpdateSeq, ch.ack.seq, qp->snd_nxt, qp->dctcp.m_alpha);
		#endif
		new_batch = true;
		if (qp->dctcp.m_lastUpdateSeq == 0){ // first RTT
			qp->dctcp.m_lastUpdateSeq = qp->snd_nxt;
			qp->dctcp.m_batchSizeOfAlpha = qp->snd_nxt / m_mtu + 1;
		}else {
			double frac = std::min(1.0, double(qp->dctcp.m_ecnCnt) / qp->dctcp.m_batchSizeOfAlpha);
			qp->dctcp.m_alpha = (1 - m_g) * qp->dctcp.m_alpha + m_g * frac;
			qp->dctcp.m_lastUpdateSeq = qp->snd_nxt;
			qp->dctcp.m_ecnCnt = 0;
			qp->dctcp.m_batchSizeOfAlpha = (qp->snd_nxt - ack_seq) / m_mtu + 1;
			#if PRINT_LOG
			printf("%.3lf F:%.3lf", qp->dctcp.m_alpha, frac);
			#endif
		}
		#if PRINT_LOG
		printf("\n");
		#endif
	}

	// check cwr exit
	if (qp->dctcp.m_caState == 1){
		if (ack_seq > qp->dctcp.m_highSeq)
			qp->dctcp.m_caState = 0;
	}

	// check if need to reduce rate: ECN and not in CWR
	if (cnp && qp->dctcp.m_caState == 0){
		#if PRINT_LOG
		printf("%lu %s %08x %08x %u %u %.3lf->", Simulator::Now().GetTimeStep(), "rate", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->m_rate.GetBitRate()*1e-9);
		#endif
		qp->m_rate = std::max(m_minRate, qp->m_rate * (1 - qp->dctcp.m_alpha / 2));
		#if PRINT_LOG
		printf("%.3lf\n", qp->m_rate.GetBitRate() * 1e-9);
		#endif
		qp->dctcp.m_caState = 1;
		qp->dctcp.m_highSeq = qp->snd_nxt;
	}

	// additive inc
	if (qp->dctcp.m_caState == 0 && new_batch)
		qp->m_rate = std::min(qp->m_max_rate, qp->m_rate + m_dctcp_rai);

	qp->PrintRate();
}

/**
 * Can handle disordered packets
 */
void RdmaHw::HandleDisOrderedAckDctcp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
	uint32_t ack_seq = ch.ack.seq;
	uint8_t cnp = (ch.ack.flags >> qbbHeader::FLAG_CNP) & 1;
	bool new_batch = false;

	uint64_t next_psn = qp->GetNxtPSN();

	// update alpha
	qp->dctcp.m_ecnCnt += (cnp > 0);
	qp->dctcp.m_acknumRTT += 1;
	if (ack_seq > qp->dctcp.m_lastUpdateSeq){ // if full RTT feedback is ready, do alpha update
		#if PRINT_LOG
		printf("%lu %s %08x %08x %u %u [%u,%u,%u] %.3lf->", Simulator::Now().GetTimeStep(), "alpha", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->dctcp.m_lastUpdateSeq, ch.ack.seq, qp->snd_nxt, qp->dctcp.m_alpha);
		#endif
		new_batch = true;
		if (qp->dctcp.m_lastUpdateSeq == 0){ // first RTT
			qp->dctcp.m_lastUpdateSeq = next_psn;
			qp->dctcp.m_batchSizeOfAlpha = qp->dctcp.m_acknumRTT;
		}else {
			double frac = std::min(1.0, double(qp->dctcp.m_ecnCnt) / qp->dctcp.m_batchSizeOfAlpha);
			qp->dctcp.m_alpha = (1 - m_g) * qp->dctcp.m_alpha + m_g * frac;
			qp->dctcp.m_lastUpdateSeq = next_psn;
			qp->dctcp.m_batchSizeOfAlpha = qp->dctcp.m_acknumRTT;
			qp->dctcp.m_ecnCnt = 0;
			qp->dctcp.m_acknumRTT = 0;
			#if PRINT_LOG
			printf("%.3lf F:%.3lf", qp->dctcp.m_alpha, frac);
			#endif
		}
		#if PRINT_LOG
		printf("\n");
		#endif
	}

	// check cwr exit
	if (qp->dctcp.m_caState == 1){
		if (ack_seq > qp->dctcp.m_highSeq)
			qp->dctcp.m_caState = 0;
	}

	// check if need to reduce rate: ECN and not in CWR
	if (cnp && qp->dctcp.m_caState == 0){
		#if PRINT_LOG
		printf("%lu %s %08x %08x %u %u %.3lf->", Simulator::Now().GetTimeStep(), "rate", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->m_rate.GetBitRate()*1e-9);
		#endif
		qp->m_rate = std::max(m_minRate, qp->m_rate * (1 - qp->dctcp.m_alpha / 2));
		#if PRINT_LOG
		printf("%.3lf\n", qp->m_rate.GetBitRate() * 1e-9);
		#endif
		qp->dctcp.m_caState = 1;
		qp->dctcp.m_highSeq = next_psn;
	}

	// additive inc
	if (qp->dctcp.m_caState == 0 && new_batch)
		qp->m_rate = std::min(qp->m_max_rate, qp->m_rate + m_dctcp_rai);

	qp->PrintRate();
}

}
