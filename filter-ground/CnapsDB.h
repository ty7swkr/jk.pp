/*
 * FilterDB.h
 *
 *  Created on: 2024. 11. 21.
 *      Author: tys
 */

#pragma once

#include <Logger.h>
#include <extra/MariaResultSet.h>
#include <extra/MariaStatement.h>
#include <extra/MariaConnectorTls.h>
#include <extra/Singleton.h>

class CnapsDBConnectors :  public MariaConnectorTls, public Singleton<CnapsDBConnectors>
{
public:
  CnapsDBConnectors()
  {
    occur_connect_error = [&](sql::SQLException &e)
    {
      sfs_log.error() << e.what();
    };

    clear_connect_error = [&]()
    {
      sfs_log.info() << "Cleard Connection Error";
    };
  }
};

#define cnaps_db CnapsDBConnectors::ref()
