/*
 * Copyright (c) 2014-17 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#include "AMQPcpp.h"
#include "RabbitMQSource.hpp"

using namespace pfabric;
  
RabbitMQSource::RabbitMQSource(const std::string& info) {
  mInfo = info;
}

RabbitMQSource::~RabbitMQSource() {
}

unsigned long RabbitMQSource::start() {
  AMQP amqp(mInfo);
  AMQPQueue* que = amqp.createQueue("q");
  que->Declare();
  uint32_t len = 0;

  //get first tuple
  que->Get(AMQP_NOACK);
  AMQPMessage* m = que->getMessage();
  char* data = m->getMessage(&len);
  produceTuple(StringRef(data, len));

  //and all the others
  while(m->getMessageCount() > 0) {
    que->Get(AMQP_NOACK);
    m = que->getMessage();
    data = m->getMessage(&len);
    produceTuple(StringRef(data, len));
  }
  return 0;
}

void RabbitMQSource::stop() {
}

void RabbitMQSource::produceTuple(const StringRef& data) {
  auto tn = makeTuplePtr(data);
  this->getOutputDataChannel().publish(tn, false);
}

void RabbitMQSource::producePunctuation(PunctuationPtr pp) {
  this->getOutputPunctuationChannel().publish(pp);
}
