#pragma once

#include "pcp_database.h"
#include "pcp_encoder.h"
#include <QString>

class PCPFormatter
{
public:
    static QString toConsoleString(const PCPFrame& frame,
                                   const QString& direction,
                                   const PCPDatabase& database);
};