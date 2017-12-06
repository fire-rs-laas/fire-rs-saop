//***************************************************************************
// Copyright 2017 OceanScan - Marine Systems & Technology, Lda.             *
//***************************************************************************
// Licensed under the Apache License, Version 2.0 (the "License");          *
// you may not use this file except in compliance with the License.         *
// You may obtain a copy of the License at                                  *
//                                                                          *
// http://www.apache.org/licenses/LICENSE-2.0                               *
//                                                                          *
// Unless required by applicable law or agreed to in writing, software      *
// distributed under the License is distributed on an "AS IS" BASIS,        *
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
// See the License for the specific language governing permissions and      *
// limitations under the License.                                           *
//***************************************************************************
// Author: Ricardo Martins                                                  *
//***************************************************************************
// Automatically generated.                                                 *
//***************************************************************************
// IMC XML MD5: 4d8734a1111656aac56f803acdc90c22                            *
//***************************************************************************

#ifndef IMC_ACOUSTICMESSAGE_HPP_INCLUDED_
#define IMC_ACOUSTICMESSAGE_HPP_INCLUDED_

// ISO C++ 98 headers.
#include <ostream>
#include <string>
#include <vector>

// IMC headers.
#include "../Base/Config.hpp"
#include "../Base/Message.hpp"
#include "../Base/InlineMessage.hpp"
#include "../Base/MessageList.hpp"
#include "../Base/JSON.hpp"
#include "../Base/Serialization.hpp"
#include "../Spec/Enumerations.hpp"
#include "../Spec/Bitfields.hpp"

namespace IMC
{
  //! Acoustic Message.
  class AcousticMessage: public Message
  {
  public:
    //! Message to send.
    InlineMessage<Message> message;

    static uint16_t
    getIdStatic(void)
    {
      return 206;
    }

    static AcousticMessage*
    cast(Message* msg__)
    {
      return (AcousticMessage*)msg__;
    }

    AcousticMessage(void)
    {
      m_header.mgid = AcousticMessage::getIdStatic();
      clear();
      message.setParent(this);
    }

    AcousticMessage*
    clone(void) const
    {
      return new AcousticMessage(*this);
    }

    void
    clear(void)
    {
      message.clear();
    }

    bool
    fieldsEqual(const Message& msg__) const
    {
      const IMC::AcousticMessage& other__ = static_cast<const AcousticMessage&>(msg__);
      if (message != other__.message) return false;
      return true;
    }

    uint8_t*
    serializeFields(uint8_t* bfr__) const
    {
      uint8_t* ptr__ = bfr__;
      ptr__ += message.serialize(ptr__);
      return ptr__;
    }

    size_t
    deserializeFields(const uint8_t* bfr__, size_t size__)
    {
      const uint8_t* start__ = bfr__;
      bfr__ += message.deserialize(bfr__, size__);
      return bfr__ - start__;
    }

    size_t
    reverseDeserializeFields(const uint8_t* bfr__, size_t size__)
    {
      const uint8_t* start__ = bfr__;
      bfr__ += message.reverseDeserialize(bfr__, size__);
      return bfr__ - start__;
    }

    uint16_t
    getId(void) const
    {
      return AcousticMessage::getIdStatic();
    }

    const char*
    getName(void) const
    {
      return "AcousticMessage";
    }

    size_t
    getFixedSerializationSize(void) const
    {
      return 0;
    }

    size_t
    getVariableSerializationSize(void) const
    {
      return message.getSerializationSize();
    }

    void
    fieldsToJSON(std::ostream& os__, unsigned nindent__) const
    {
      message.toJSON(os__, "message", nindent__);
    }

  protected:
    void
    setTimeStampNested(double value__)
    {
      if (!message.isNull())
      {
        message.get()->setTimeStamp(value__);
      }
    }

    void
    setSourceNested(uint16_t value__)
    {
      if (!message.isNull())
      {
        message.get()->setSource(value__);
      }
    }

    void
    setSourceEntityNested(uint8_t value__)
    {
      if (!message.isNull())
      {
        message.get()->setSourceEntity(value__);
      }
    }

    void
    setDestinationNested(uint16_t value__)
    {
      if (!message.isNull())
      {
        message.get()->setDestination(value__);
      }
    }

    void
    setDestinationEntityNested(uint8_t value__)
    {
      if (!message.isNull())
      {
        message.get()->setDestinationEntity(value__);
      }
    }
  };
}

#endif