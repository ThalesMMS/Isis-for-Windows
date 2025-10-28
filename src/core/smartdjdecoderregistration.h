#pragma once

#include "utils.h"

namespace isis::core
{
    class export SmartDJDecoderRegistration
    {
    public:
        SmartDJDecoderRegistration() = default;
        ~SmartDJDecoderRegistration() = default;

        static void registerCodecs();
        static void cleanup();
    };
}
