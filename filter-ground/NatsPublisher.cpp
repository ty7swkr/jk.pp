/*
 * NatsClientThread.h
 *
 *  Created on: 2024. 12. 3.
 *      Author: tys
 */

#include "NatsPublisher.h"
#include <extra/ThreadUniqueIndexer.h>
#include <Logger.h>
#include <extra/Toggle.h>

static void
nats_error_callback(natsConnection *nc, natsSubscription *sub, natsStatus err, void *closure)
{
  (void)nc; (void)sub; (void)err; (void)closure;
}

bool
NatsPublisher::start()
{
  Toggle error_toggle(false, false);

  try
  {
    client_ = std::unique_ptr<NatsClient>(new NatsClient());
    client_->connectServers(urls_.load(), nats_error_callback);
  }
  catch (const SfsNatsException &e)
  {
    if (error_toggle.turn_on() == true)
      sfs_log.error() << e.what();
    return false;
  }
  catch (const std::exception &e)
  {
    if (error_toggle.turn_on() == true)
      sfs_log.error() << e.what();
    return false;
  }

  if (error_toggle.turn_off() == true)
    sfs_log.info() << "NATS transmission error cleared.";

  return LockFreeQueueThread::start();
}

void
NatsPublisher::run()
{
  Toggle error_toggle(false, false);

  sfs_log.info() << "Start Publisher:" << to_stringf(assigned_no_, "%02d");

  queueable_pair item;// = 0;

  while (waiter_.pop(item) >= 0)
  {
    // subject, message(json)
    std::shared_ptr<std::pair<std::string, std::string>> pair = item.take();

    try
    {
      client_->publish(pair->first, pair->second);
    }
    catch (const SfsNatsException &e)
    {
      if (error_toggle.turn_on() == true)
        sfs_log.error() << pair->second + ": " + e.what();
      continue;
    }

    if (error_toggle.turn_off() == true)
      sfs_log.info() << pair->second + ": NATS transmission error cleared.";
  }

  client_->flush();
  client_.reset();

  sfs_log.info() << "Stop Publisher:" << to_stringf(assigned_no_, "%02d");
}







