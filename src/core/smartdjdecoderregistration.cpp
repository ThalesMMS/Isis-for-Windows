#include "smartdjdecoderregistration.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcSmartDecoderRegistration, "isis.core.codec")

void isis::core::SmartDJDecoderRegistration::registerCodecs()
{
        qCDebug(lcSmartDecoderRegistration) << "GDCM pipeline active; DCMTK codec registration skipped.";
}

void isis::core::SmartDJDecoderRegistration::cleanup()
{
        qCDebug(lcSmartDecoderRegistration) << "GDCM pipeline active; DCMTK codec cleanup skipped.";
}
