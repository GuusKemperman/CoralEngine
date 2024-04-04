#include "Precomp.h"
#include "Assets/Importers/Importer.h"

#include "Meta/MetaType.h"

CE::MetaType CE::Importer::Reflect()
{
    return MetaType{ MetaType::T<Importer>{}, "Importer" };
}
