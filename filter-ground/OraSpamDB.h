/*
 * OraSpamDB.h
 *
 *  Created on: 2024. 12. 17.
 *      Author: tys
 */

#pragma once

#include <Logger.h>
#include <extra/OtlConnectorTls.h>
#include <extra/Singleton.h>

class OraSpamDBConnectors :  public OtlConnectorTls, public Singleton<OraSpamDBConnectors>
{
public:
  OraSpamDBConnectors()
  {
    occur_connect_error = [&](const otl_exception &e)
    {
      sfs_log.error() << (const char *)e.msg;
    };

    clear_connect_error = [&]()
    {
      sfs_log.info() << "Cleared Connect Error";
    };
  }
};

#define oraspam_db OraSpamDBConnectors::ref()


