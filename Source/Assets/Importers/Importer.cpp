#include "Precomp.h"
#include "Assets/Importers/Importer.h"

#include "Meta/MetaType.h"

Engine::MetaType Engine::Importer::Reflect()
{
    return MetaType{ MetaType::T<Importer>{}, "Importer" };
}
