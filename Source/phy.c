#include "config.h"
#include "util.h"

#ifdef LAN_NODE
uint8_t macaddr[6];
uint8_t ipaddr[4];

// eth/ip buffer:
#define MAX_FRAME_BUF 350
static uint8_t buf[MAX_FRAME_BUF];
#endif  //  LAN_NODE

void PHY_Init(void)
{
#ifdef ENC28J60
  //initialize enc28j60
  enc28j60Init(macaddr);
#endif
#ifdef RF_NODE
  rf_Initialize();
#endif  //  RF_NODE
}

void PHY_LoadConfig(void)
{
  uint8_t Len;

#ifdef RF_NODE
  // (Re)Load RF Configuration
  uint16_t uiTmp;
  uint8_t channel, NodeID;
  Len = sizeof(uint16_t);
  // Load config data
  ReadOD(objRFGroup, MQTTS_FL_TOPICID_PREDEF,   &Len, (uint8_t *)&uiTmp);
  ReadOD(objRFNodeId, MQTTS_FL_TOPICID_PREDEF,  &Len, &NodeID);
  ReadOD(objRFChannel, MQTTS_FL_TOPICID_PREDEF, &Len, &channel);
  rf_LoadCfg(channel, uiTmp, NodeID);
#endif  //  RF_NODE

#ifdef LAN_NODE
  Len = 6;
  ReadOD(objMACAddr, MQTTS_FL_TOPICID_PREDEF, &Len, (uint8_t *)macaddr);
  Len = 4;
  ReadOD(objIPAddr, MQTTS_FL_TOPICID_PREDEF, &Len, (uint8_t *)ipaddr);
//  ReadOD(objIPBroker, MQTTS_FL_TOPICID_PREDEF, &Len, (uint8_t *)gwip);
#endif  //  LAN_NODE
}

void PHY_Pool(void)
{
#ifdef RF_NODE
  rf_Pool();
#endif  // RF_NODE
#ifdef LAN_NODE
  static uint8_t PoolCnt = POOL_TMR_FREQ - 1;

  if(PoolCnt)
    PoolCnt--;
  else
  {
    PoolCnt = POOL_TMR_FREQ - 1;
    sec_tick_lan();
  }
#endif  //  LAN_NODE
}

void PHY_Start(void)
{
#ifdef LAN_NODE
#ifdef  DHCP_client
  if(ipaddr[0] == 0xFF)
  {
    // DHCP handling. Get the initial IP
    uint8_t i;
    uint16_t plen;

    i = 0;
    while(i == 0)
    {
      plen = enc28j60PacketReceive(MAX_FRAME_BUF, buf);
      i = packetloop_dhcp_initial_ip_assignment(buf, plen);
    }
  }
#else
  while (enc28j60linkup()==0)
    _delay_ms(250);
#endif  //  DHCP_client
#endif  //  LAN_NODE
}

MQ_t * PHY_GetBuf(s_Addr * pSrcAddr)
{
#ifdef LAN_NODE
  uint16_t plen;
  MQ_t * pTmp;

  plen = enc28j60PacketReceive(MAX_FRAME_BUF, buf);
  if(packetloop_lan(buf, plen))    // 1 - Unicast, 2 - Broadcast
  {
    if((buf[UDP_DST_PORT_H_P] == (MQTTS_UDP_PORT >> 8)) &&
       (buf[UDP_DST_PORT_L_P] == (MQTTS_UDP_PORT & 0xFF)))
    {
      memcpy(&pSrcAddr->mac, &buf[ETH_SRC_MAC], 6);
      memcpy(&pSrcAddr->ip,  &buf[IP_SRC_P], 4);
      
      plen -= UDP_DATA_P;
      if((plen > sizeof(MQ_t)) || ((pTmp = mqAssert()) == NULL))
        return NULL;
      memcpy(pTmp, &buf[UDP_DATA_P], plen);
      return pTmp;
    }
  }
  return NULL;
#endif  //  LAN_NODE
#ifdef RF_NODE
  return (MQ_t *)rf_GetBuf(pSrcAddr);
#endif  //  RF_NODE
}

void PHY_Send(MQ_t * pBuf, s_Addr * pDstAddr)
{
#ifdef LAN_NODE
  send_udp_prepare(buf, MQTTS_UDP_PORT, pDstAddr->ip, MQTTS_UDP_PORT, pDstAddr->mac);
  
  uint16_t datalen = pBuf->Length;

  memcpy(&buf[UDP_DATA_P], pBuf, datalen);
  mqRelease(pBuf);
  
  send_udp_transmit(buf, datalen);
#endif  //  LAN_NODE
#ifdef RF_NODE
  rf_Send((uint8_t *)pBuf, pDstAddr);
#endif  //  RF_NODE
}

uint8_t PHY_BuildName(uint8_t * pBuf)
{
#ifdef LAN_NODE
  sprinthex(&pBuf[0], ipaddr[0]);
  sprinthex(&pBuf[2], ipaddr[1]);
  sprinthex(&pBuf[4], ipaddr[2]);
  sprinthex(&pBuf[6], ipaddr[3]);

  return 8;
#endif  //  LAN_NODE
#ifdef RF_NODE
  sprinthex(&pBuf[0], rf_GetNodeID());
  return 2;
#endif  //  RF_NODE
}