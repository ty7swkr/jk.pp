/*
 * FilterTpsMeter.h
 *
 *  Created on: 2024. 12. 4.
 *      Author: tys
 */

#pragma once

#include <extra/TpsMeter.h>
#include <extra/Singleton.h>

#define tps_meter_in  TpsMeterIn ::ref()
#define tps_meter_out TpsMeterOut::ref()

class TpsMeterIn  : public TpsMeter, public Singleton<TpsMeterIn> {};
class TpsMeterOut : public TpsMeter, public Singleton<TpsMeterOut> {};


