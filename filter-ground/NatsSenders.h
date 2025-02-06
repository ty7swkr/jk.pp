/*
 * NatsSenders.h
 *
 *  Created on: 2024. 12. 3.
 *      Author: tys
 */

#pragma once

#include <NatsPublisherPool.h>
#include <extra/Singleton.h>

class NatsSender  : public NatsPublisherPool, public Singleton<NatsSender> {};
class NatsResult  : public NatsPublisherPool, public Singleton<NatsResult> {};

#define nats_sender NatsSender::ref()
#define nats_result NatsResult::ref()


