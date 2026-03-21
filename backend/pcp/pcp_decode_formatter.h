#pragma once

#include "pcp_decoder.h"
#include <QString>

class PCPDecodeFormatter
{
public:
    static QString toText(const PCPDecodedMessage& msg);
};