// Copyright (c) 2026 Capgemini Engineering Research and Development.
//
// This file is part of OCCT-Light software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License version 3 as published
// by the Free Software Foundation, with an option to use any later version.
// Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution
// for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of a commercial
// license or contractual agreement.
//
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "GraphHandle.hxx"
#include "GraphLayers.hxx"
#include "TopoMath.hxx"

#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include <BRepGraph_Layer.hxx>
#include <BRepGraph_LayerRegistry.hxx>
#include <BRepGraph_TopoView.hxx>
#include <BRepGraph_UIDsView.hxx>
#include <NCollection_DataMap.hxx>
#include <NCollection_FlatDataMap.hxx>
#include <NCollection_FlatMap.hxx>
#include <NCollection_LinearVector.hxx>
#include <Standard_GUID.hxx>
#include <TCollection_AsciiString.hxx>
#include <Precision.hxx>
#include <Quantity_ColorRGBA.hxx>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace
{

bool IsFiniteValue(const double theValue)
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

bool IsFiniteValue(const float theValue)
{
  return !Precision::IsInfinite(static_cast<double>(theValue)) && !std::isnan(theValue);
}

size_t asciiLength(const TCollection_AsciiString& theText)
{
  return static_cast<size_t>(theText.Length());
}

bool isActiveNode(const BRepGraph& theGraph, BRepGraph_NodeId theNode);

class OcctL_Topo_ColorLayer : public BRepGraph_Layer
{
public:
  static const Standard_GUID& GetID()
  {
    static const Standard_GUID THE_ID("8cbdca4a-34a1-4ad4-902a-e8f27696fc40");
    return THE_ID;
  }

  const Standard_GUID& ID() const override { return GetID(); }

  const TCollection_AsciiString& Name() const override
  {
    static const TCollection_AsciiString THE_NAME("OCCT-Light Color");
    return THE_NAME;
  }

  void SetColor(const BRepGraph_NodeId theNode, const Quantity_ColorRGBA& theColor)
  {
    const Quantity_ColorRGBA* aCurrent = myColors.Seek(theNode);
    if (aCurrent != nullptr && aCurrent->IsEqual(theColor))
    {
      return;
    }
    myColors.Bind(theNode, theColor);
    touch();
  }

  bool FindColor(const BRepGraph_NodeId theNode, Quantity_ColorRGBA& theColor) const
  {
    const Quantity_ColorRGBA* aColor = myColors.Seek(theNode);
    if (aColor == nullptr)
    {
      return false;
    }
    theColor = *aColor;
    return true;
  }

  void CollectActive(const BRepGraph&                              theGraph,
                     NCollection_LinearVector<BRepGraph_NodeId>&   theNodes,
                     NCollection_LinearVector<Quantity_ColorRGBA>& theColors) const
  {
    for (NCollection_DataMap<BRepGraph_NodeId, Quantity_ColorRGBA>::Iterator anIter(myColors);
         anIter.More();
         anIter.Next())
    {
      if (!isActiveNode(theGraph, anIter.Key()))
      {
        continue;
      }
      theNodes.Append(anIter.Key());
      theColors.Append(anIter.Value());
    }
  }

  void CopyTo(const BRepGraph&       theSource,
              BRepGraph&             theTarget,
              OcctL_Topo_ColorLayer& theTargetLayer) const
  {
    for (NCollection_DataMap<BRepGraph_NodeId, Quantity_ColorRGBA>::Iterator anIter(myColors);
         anIter.More();
         anIter.Next())
    {
      const BRepGraph_UID    aUid        = theSource.UIDs().Of(anIter.Key());
      const BRepGraph_NodeId aTargetNode = theTarget.UIDs().NodeIdFrom(aUid);
      if (aTargetNode.IsValid())
      {
        theTargetLayer.SetColor(aTargetNode, anIter.Value());
      }
    }
  }

  void RemoveColor(const BRepGraph_NodeId theNode)
  {
    if (myColors.UnBind(theNode))
    {
      touch();
    }
  }

  void OnNodeRemoved(const BRepGraph_NodeId theNode) noexcept override
  {
    OnNodeReplaced(theNode, BRepGraph_NodeId());
  }

  void OnNodeReplaced(const BRepGraph_NodeId theNode,
                      const BRepGraph_NodeId theReplacement) noexcept override
  {
    const Quantity_ColorRGBA* anOldColor = myColors.Seek(theNode);
    if (anOldColor == nullptr)
    {
      return;
    }
    if (theReplacement.IsValid() && myColors.Seek(theReplacement) == nullptr)
    {
      myColors.Bind(theReplacement, *anOldColor);
    }
    myColors.UnBind(theNode);
    touch();
  }

  void OnCompact(
    const NCollection_DataMap<BRepGraph_NodeId, BRepGraph_NodeId>& theRemapMap) noexcept override
  {
    bool                                                      hasTopologyColors = false;
    NCollection_DataMap<BRepGraph_NodeId, Quantity_ColorRGBA> aRemapped;
    for (NCollection_DataMap<BRepGraph_NodeId, Quantity_ColorRGBA>::Iterator anIter(myColors);
         anIter.More();
         anIter.Next())
    {
      const BRepGraph_NodeId& anOldId = anIter.Key();
      if (BRepGraph_NodeId::IsTopologyKind(anOldId.NodeKind))
      {
        hasTopologyColors              = true;
        const BRepGraph_NodeId* aNewId = theRemapMap.Seek(anOldId);
        if (aNewId != nullptr)
        {
          aRemapped.Bind(*aNewId, anIter.Value());
        }
      }
      else
      {
        aRemapped.Bind(anOldId, anIter.Value());
      }
    }
    myColors = std::move(aRemapped);
    if (hasTopologyColors)
    {
      touch();
    }
  }

  void InvalidateAll() noexcept override {}

  void Clear() noexcept override
  {
    const bool hadData = !myColors.IsEmpty();
    myColors.Clear();
    if (hadData)
    {
      touch();
    }
  }

  DEFINE_STANDARD_RTTIEXT(OcctL_Topo_ColorLayer, BRepGraph_Layer)

private:
  NCollection_DataMap<BRepGraph_NodeId, Quantity_ColorRGBA> myColors;
};

class OcctL_Topo_NameLayer : public BRepGraph_Layer
{
public:
  static const Standard_GUID& GetID()
  {
    static const Standard_GUID THE_ID("9348f1ea-0db3-4f28-884c-9ed186ee2013");
    return THE_ID;
  }

  const Standard_GUID& ID() const override { return GetID(); }

  const TCollection_AsciiString& Name() const override
  {
    static const TCollection_AsciiString THE_NAME("OCCT-Light Name");
    return THE_NAME;
  }

  void SetName(const BRepGraph_NodeId theNode, const TCollection_AsciiString& theName)
  {
    const TCollection_AsciiString* aCurrent = myNames.Seek(theNode);
    if (aCurrent != nullptr && aCurrent->IsEqual(theName))
    {
      return;
    }
    myNames.Bind(theNode, theName);
    touch();
  }

  const TCollection_AsciiString* FindName(const BRepGraph_NodeId theNode) const
  {
    return myNames.Seek(theNode);
  }

  void CollectActive(const BRepGraph&                            theGraph,
                     NCollection_LinearVector<BRepGraph_NodeId>& theNodes) const
  {
    for (NCollection_DataMap<BRepGraph_NodeId, TCollection_AsciiString>::Iterator anIter(myNames);
         anIter.More();
         anIter.Next())
    {
      if (isActiveNode(theGraph, anIter.Key()))
      {
        theNodes.Append(anIter.Key());
      }
    }
  }

  void CopyTo(const BRepGraph&      theSource,
              BRepGraph&            theTarget,
              OcctL_Topo_NameLayer& theTargetLayer) const
  {
    for (NCollection_DataMap<BRepGraph_NodeId, TCollection_AsciiString>::Iterator anIter(myNames);
         anIter.More();
         anIter.Next())
    {
      const BRepGraph_UID    aUid        = theSource.UIDs().Of(anIter.Key());
      const BRepGraph_NodeId aTargetNode = theTarget.UIDs().NodeIdFrom(aUid);
      if (aTargetNode.IsValid())
      {
        theTargetLayer.SetName(aTargetNode, anIter.Value());
      }
    }
  }

  void RemoveName(const BRepGraph_NodeId theNode)
  {
    if (myNames.UnBind(theNode))
    {
      touch();
    }
  }

  void OnNodeRemoved(const BRepGraph_NodeId theNode) noexcept override
  {
    OnNodeReplaced(theNode, BRepGraph_NodeId());
  }

  void OnNodeReplaced(const BRepGraph_NodeId theNode,
                      const BRepGraph_NodeId theReplacement) noexcept override
  {
    const TCollection_AsciiString* anOldName = myNames.Seek(theNode);
    if (anOldName == nullptr)
    {
      return;
    }
    if (theReplacement.IsValid() && myNames.Seek(theReplacement) == nullptr)
    {
      myNames.Bind(theReplacement, *anOldName);
    }
    myNames.UnBind(theNode);
    touch();
  }

  void OnCompact(
    const NCollection_DataMap<BRepGraph_NodeId, BRepGraph_NodeId>& theRemapMap) noexcept override
  {
    bool                                                           hasTopologyNames = false;
    NCollection_DataMap<BRepGraph_NodeId, TCollection_AsciiString> aRemapped;
    for (NCollection_DataMap<BRepGraph_NodeId, TCollection_AsciiString>::Iterator anIter(myNames);
         anIter.More();
         anIter.Next())
    {
      const BRepGraph_NodeId& anOldId = anIter.Key();
      if (BRepGraph_NodeId::IsTopologyKind(anOldId.NodeKind))
      {
        hasTopologyNames               = true;
        const BRepGraph_NodeId* aNewId = theRemapMap.Seek(anOldId);
        if (aNewId != nullptr)
        {
          aRemapped.Bind(*aNewId, anIter.Value());
        }
      }
      else
      {
        aRemapped.Bind(anOldId, anIter.Value());
      }
    }
    myNames = std::move(aRemapped);
    if (hasTopologyNames)
    {
      touch();
    }
  }

  void InvalidateAll() noexcept override {}

  void Clear() noexcept override
  {
    const bool hadData = !myNames.IsEmpty();
    myNames.Clear();
    if (hadData)
    {
      touch();
    }
  }

  DEFINE_STANDARD_RTTIEXT(OcctL_Topo_NameLayer, BRepGraph_Layer)

private:
  NCollection_DataMap<BRepGraph_NodeId, TCollection_AsciiString> myNames;
};

struct MaterialRecord
{
  TCollection_AsciiString Name;
  bool                    HasDensity      = false;
  double                  Density         = 0.0;
  bool                    HasDiffuseColor = false;
  Quantity_ColorRGBA      DiffuseColor;
  occtl_uid_t             MetadataUid = OCCTL_UID_INVALID;
};

class OcctL_Topo_MaterialLayer : public BRepGraph_Layer
{
public:
  static const Standard_GUID& GetID()
  {
    static const Standard_GUID THE_ID("1dc04cb6-97e5-4fc3-a30b-48d6169651f8");
    return THE_ID;
  }

  const Standard_GUID& ID() const override { return GetID(); }

  const TCollection_AsciiString& Name() const override
  {
    static const TCollection_AsciiString THE_NAME("OCCT-Light Material");
    return THE_NAME;
  }

  void SetMaterial(const BRepGraph_NodeId theNode, const MaterialRecord& theRecord)
  {
    const MaterialRecord* aCurrent = myMaterials.Seek(theNode);
    if (aCurrent != nullptr && aCurrent->Name == theRecord.Name
        && aCurrent->HasDensity == theRecord.HasDensity && aCurrent->Density == theRecord.Density
        && aCurrent->HasDiffuseColor == theRecord.HasDiffuseColor
        && (!theRecord.HasDiffuseColor || aCurrent->DiffuseColor.IsEqual(theRecord.DiffuseColor))
        && aCurrent->MetadataUid.bits == theRecord.MetadataUid.bits)
    {
      return;
    }
    myMaterials.Bind(theNode, theRecord);
    touch();
  }

  const MaterialRecord* FindMaterial(const BRepGraph_NodeId theNode) const
  {
    return myMaterials.Seek(theNode);
  }

  void CollectActive(const BRepGraph&                            theGraph,
                     NCollection_LinearVector<BRepGraph_NodeId>& theNodes) const
  {
    for (NCollection_DataMap<BRepGraph_NodeId, MaterialRecord>::Iterator anIter(myMaterials);
         anIter.More();
         anIter.Next())
    {
      if (isActiveNode(theGraph, anIter.Key()))
      {
        theNodes.Append(anIter.Key());
      }
    }
  }

  void CopyTo(const BRepGraph&          theSource,
              BRepGraph&                theTarget,
              OcctL_Topo_MaterialLayer& theTargetLayer) const
  {
    for (NCollection_DataMap<BRepGraph_NodeId, MaterialRecord>::Iterator anIter(myMaterials);
         anIter.More();
         anIter.Next())
    {
      const BRepGraph_UID    aUid        = theSource.UIDs().Of(anIter.Key());
      const BRepGraph_NodeId aTargetNode = theTarget.UIDs().NodeIdFrom(aUid);
      if (aTargetNode.IsValid())
      {
        theTargetLayer.SetMaterial(aTargetNode, anIter.Value());
      }
    }
  }

  void RemoveMaterial(const BRepGraph_NodeId theNode)
  {
    if (myMaterials.UnBind(theNode))
    {
      touch();
    }
  }

  void OnNodeRemoved(const BRepGraph_NodeId theNode) noexcept override
  {
    OnNodeReplaced(theNode, BRepGraph_NodeId());
  }

  void OnNodeReplaced(const BRepGraph_NodeId theNode,
                      const BRepGraph_NodeId theReplacement) noexcept override
  {
    const MaterialRecord* anOldMaterial = myMaterials.Seek(theNode);
    if (anOldMaterial == nullptr)
    {
      return;
    }
    if (theReplacement.IsValid() && myMaterials.Seek(theReplacement) == nullptr)
    {
      myMaterials.Bind(theReplacement, *anOldMaterial);
    }
    myMaterials.UnBind(theNode);
    touch();
  }

  void OnCompact(
    const NCollection_DataMap<BRepGraph_NodeId, BRepGraph_NodeId>& theRemapMap) noexcept override
  {
    bool                                                  hasTopologyMaterials = false;
    NCollection_DataMap<BRepGraph_NodeId, MaterialRecord> aRemapped;
    for (NCollection_DataMap<BRepGraph_NodeId, MaterialRecord>::Iterator anIter(myMaterials);
         anIter.More();
         anIter.Next())
    {
      const BRepGraph_NodeId& anOldId = anIter.Key();
      if (BRepGraph_NodeId::IsTopologyKind(anOldId.NodeKind))
      {
        hasTopologyMaterials           = true;
        const BRepGraph_NodeId* aNewId = theRemapMap.Seek(anOldId);
        if (aNewId != nullptr)
        {
          aRemapped.Bind(*aNewId, anIter.Value());
        }
      }
      else
      {
        aRemapped.Bind(anOldId, anIter.Value());
      }
    }
    myMaterials = std::move(aRemapped);
    if (hasTopologyMaterials)
    {
      touch();
    }
  }

  void InvalidateAll() noexcept override {}

  void Clear() noexcept override
  {
    const bool hadData = !myMaterials.IsEmpty();
    myMaterials.Clear();
    if (hadData)
    {
      touch();
    }
  }

  DEFINE_STANDARD_RTTIEXT(OcctL_Topo_MaterialLayer, BRepGraph_Layer)

private:
  NCollection_DataMap<BRepGraph_NodeId, MaterialRecord> myMaterials;
};

class OcctL_Topo_UnitsLayer : public BRepGraph_Layer
{
public:
  static const Standard_GUID& GetID()
  {
    static const Standard_GUID THE_ID("9b87e36a-a462-493a-8a0f-5f2eb8679db2");
    return THE_ID;
  }

  const Standard_GUID& ID() const override { return GetID(); }

  const TCollection_AsciiString& Name() const override
  {
    static const TCollection_AsciiString THE_NAME("OCCT-Light Units");
    return THE_NAME;
  }

  void SetUnits(const double theLengthUnitToMeter, const TCollection_AsciiString& theName)
  {
    if (myHasUnits && myLengthUnitToMeter == theLengthUnitToMeter && myName.IsEqual(theName))
    {
      return;
    }
    myHasUnits          = true;
    myLengthUnitToMeter = theLengthUnitToMeter;
    myName              = theName;
    touch();
  }

  bool FindUnits(double& theLengthUnitToMeter, TCollection_AsciiString& theName) const
  {
    if (!myHasUnits)
    {
      return false;
    }
    theLengthUnitToMeter = myLengthUnitToMeter;
    theName              = myName;
    return true;
  }

  void CopyTo(OcctL_Topo_UnitsLayer& theTargetLayer) const
  {
    if (myHasUnits)
    {
      theTargetLayer.SetUnits(myLengthUnitToMeter, myName);
    }
  }

  void OnNodeRemoved(const BRepGraph_NodeId) noexcept override {}

  void OnNodeReplaced(const BRepGraph_NodeId, const BRepGraph_NodeId) noexcept override {}

  void OnCompact(const NCollection_DataMap<BRepGraph_NodeId, BRepGraph_NodeId>&) noexcept override
  {
  }

  void InvalidateAll() noexcept override {}

  void Clear() noexcept override
  {
    if (myHasUnits)
    {
      myHasUnits          = false;
      myLengthUnitToMeter = 1.0;
      myName.Clear();
      touch();
    }
  }

  DEFINE_STANDARD_RTTIEXT(OcctL_Topo_UnitsLayer, BRepGraph_Layer)

private:
  bool                    myHasUnits          = false;
  double                  myLengthUnitToMeter = 1.0;
  TCollection_AsciiString myName;
};

class OcctL_Topo_MetadataLayer : public BRepGraph_Layer
{
public:
  static const Standard_GUID& GetID()
  {
    static const Standard_GUID THE_ID("24aca0a3-6e82-42c1-9730-0909546a765a");
    return THE_ID;
  }

  const Standard_GUID& ID() const override { return GetID(); }

  const TCollection_AsciiString& Name() const override
  {
    static const TCollection_AsciiString THE_NAME("OCCT-Light Metadata");
    return THE_NAME;
  }

  void SetValue(const BRepGraph_NodeId         theNode,
                const TCollection_AsciiString& theKey,
                const TCollection_AsciiString& theValue)
  {
    NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>* aNodeMetadataPtr =
      myValues.ChangeSeek(theNode);
    if (aNodeMetadataPtr == nullptr)
    {
      myValues.Bind(theNode,
                    NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>());
      aNodeMetadataPtr = myValues.ChangeSeek(theNode);
    }
    NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>& aNodeMetadata =
      *aNodeMetadataPtr;
    const TCollection_AsciiString* anExisting = aNodeMetadata.Seek(theKey);
    if (anExisting != nullptr && anExisting->IsEqual(theValue))
    {
      return;
    }
    TCollection_AsciiString* aValue = aNodeMetadata.ChangeSeek(theKey);
    if (aValue == nullptr)
    {
      aNodeMetadata.Bind(theKey, theValue);
    }
    else
    {
      *aValue = theValue;
    }
    touch();
  }

  void SetGraphValue(const TCollection_AsciiString& theKey, const TCollection_AsciiString& theValue)
  {
    const TCollection_AsciiString* anExisting = myGraphValues.Seek(theKey);
    if (anExisting != nullptr && anExisting->IsEqual(theValue))
    {
      return;
    }
    TCollection_AsciiString* aValue = myGraphValues.ChangeSeek(theKey);
    if (aValue == nullptr)
    {
      myGraphValues.Bind(theKey, theValue);
    }
    else
    {
      *aValue = theValue;
    }
    touch();
  }

  const TCollection_AsciiString* FindValue(const BRepGraph_NodeId         theNode,
                                           const TCollection_AsciiString& theKey) const
  {
    const NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>* aNodeMetadata =
      myValues.Seek(theNode);
    if (aNodeMetadata == nullptr)
    {
      return nullptr;
    }
    return aNodeMetadata->Seek(theKey);
  }

  const TCollection_AsciiString* FindGraphValue(const TCollection_AsciiString& theKey) const
  {
    return myGraphValues.Seek(theKey);
  }

  void CollectKeys(const BRepGraph_NodeId                               theNode,
                   NCollection_LinearVector<occtl_metadata_key_view_t>& theKeys) const
  {
    const NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>* aNodeMetadata =
      myValues.Seek(theNode);
    if (aNodeMetadata == nullptr)
    {
      return;
    }
    for (NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>::Iterator anIter(
           *aNodeMetadata);
         anIter.More();
         anIter.Next())
    {
      const TCollection_AsciiString& aKey = anIter.Key();
      occtl_metadata_key_view_t      aView;
      aView.key     = aKey.ToCString();
      aView.key_len = asciiLength(aKey);
      theKeys.Append(aView);
    }
  }

  void CollectGraphKeys(NCollection_LinearVector<occtl_metadata_key_view_t>& theKeys) const
  {
    for (NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>::Iterator anIter(
           myGraphValues);
         anIter.More();
         anIter.Next())
    {
      const TCollection_AsciiString& aKey = anIter.Key();
      occtl_metadata_key_view_t      aView;
      aView.key     = aKey.ToCString();
      aView.key_len = asciiLength(aKey);
      theKeys.Append(aView);
    }
  }

  void CollectActive(const BRepGraph&                            theGraph,
                     NCollection_LinearVector<BRepGraph_NodeId>& theNodes) const
  {
    for (NCollection_DataMap<
           BRepGraph_NodeId,
           NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>>::Iterator
           anIter(myValues);
         anIter.More();
         anIter.Next())
    {
      if (isActiveNode(theGraph, anIter.Key()) && !anIter.Value().IsEmpty())
      {
        theNodes.Append(anIter.Key());
      }
    }
  }

  void RemoveGraphValue(const TCollection_AsciiString& theKey)
  {
    if (myGraphValues.UnBind(theKey))
    {
      touch();
    }
  }

  void RemoveValue(const BRepGraph_NodeId theNode, const TCollection_AsciiString& theKey)
  {
    NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>* aNodeMetadata =
      myValues.ChangeSeek(theNode);
    if (aNodeMetadata == nullptr)
    {
      return;
    }
    const bool aRemoved = aNodeMetadata->UnBind(theKey);
    if (aNodeMetadata->IsEmpty())
    {
      myValues.UnBind(theNode);
    }
    if (aRemoved)
    {
      touch();
    }
  }

  void CopyTo(const BRepGraph&          theSource,
              BRepGraph&                theTarget,
              OcctL_Topo_MetadataLayer& theTargetLayer) const
  {
    for (NCollection_DataMap<
           BRepGraph_NodeId,
           NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>>::Iterator
           anIter(myValues);
         anIter.More();
         anIter.Next())
    {
      const BRepGraph_UID    aUid        = theSource.UIDs().Of(anIter.Key());
      const BRepGraph_NodeId aTargetNode = theTarget.UIDs().NodeIdFrom(aUid);
      if (!aTargetNode.IsValid())
      {
        continue;
      }
      for (NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>::Iterator
             aValueIter(anIter.Value());
           aValueIter.More();
           aValueIter.Next())
      {
        theTargetLayer.SetValue(aTargetNode, aValueIter.Key(), aValueIter.Value());
      }
    }
    for (NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>::Iterator anIter(
           myGraphValues);
         anIter.More();
         anIter.Next())
    {
      theTargetLayer.SetGraphValue(anIter.Key(), anIter.Value());
    }
  }

  void OnNodeRemoved(const BRepGraph_NodeId theNode) noexcept override
  {
    OnNodeReplaced(theNode, BRepGraph_NodeId());
  }

  void OnNodeReplaced(const BRepGraph_NodeId theNode,
                      const BRepGraph_NodeId theReplacement) noexcept override
  {
    const NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>* anOldValues =
      myValues.Seek(theNode);
    if (anOldValues == nullptr)
    {
      return;
    }
    if (theReplacement.IsValid())
    {
      NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>* aReplacementValuesPtr =
        myValues.ChangeSeek(theReplacement);
      if (aReplacementValuesPtr == nullptr)
      {
        myValues.Bind(theReplacement,
                      NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>());
        aReplacementValuesPtr = myValues.ChangeSeek(theReplacement);
      }
      NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>& aReplacementValues =
        *aReplacementValuesPtr;
      for (NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>::Iterator anIter(
             *anOldValues);
           anIter.More();
           anIter.Next())
      {
        if (aReplacementValues.Seek(anIter.Key()) == nullptr)
        {
          aReplacementValues.Bind(anIter.Key(), anIter.Value());
        }
      }
    }
    myValues.UnBind(theNode);
    touch();
  }

  void OnCompact(
    const NCollection_DataMap<BRepGraph_NodeId, BRepGraph_NodeId>& theRemapMap) noexcept override
  {
    bool hasTopologyMetadata = false;
    NCollection_DataMap<BRepGraph_NodeId,
                        NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>>
      aRemapped;
    for (NCollection_DataMap<
           BRepGraph_NodeId,
           NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>>::Iterator
           anIter(myValues);
         anIter.More();
         anIter.Next())
    {
      const BRepGraph_NodeId& anOldId = anIter.Key();
      if (BRepGraph_NodeId::IsTopologyKind(anOldId.NodeKind))
      {
        hasTopologyMetadata            = true;
        const BRepGraph_NodeId* aNewId = theRemapMap.Seek(anOldId);
        if (aNewId != nullptr)
        {
          aRemapped.Bind(*aNewId, anIter.Value());
        }
      }
      else
      {
        aRemapped.Bind(anOldId, anIter.Value());
      }
    }
    myValues = std::move(aRemapped);
    if (hasTopologyMetadata)
    {
      touch();
    }
  }

  void InvalidateAll() noexcept override {}

  void Clear() noexcept override
  {
    const bool hadData = !myValues.IsEmpty() || !myGraphValues.IsEmpty();
    myValues.Clear();
    myGraphValues.Clear();
    if (hadData)
    {
      touch();
    }
  }

  DEFINE_STANDARD_RTTIEXT(OcctL_Topo_MetadataLayer, BRepGraph_Layer)

private:
  NCollection_DataMap<BRepGraph_NodeId,
                      NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString>>
                                                                        myValues;
  NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString> myGraphValues;
};

class OcctL_Topo_TagLayer : public BRepGraph_Layer
{
public:
  static const Standard_GUID& GetID()
  {
    static const Standard_GUID THE_ID("3ef74e58-6a8a-4cd4-9c4c-fad92d0eb7d8");
    return THE_ID;
  }

  const Standard_GUID& ID() const override { return GetID(); }

  const TCollection_AsciiString& Name() const override
  {
    static const TCollection_AsciiString THE_NAME("OCCT-Light Tags");
    return THE_NAME;
  }

  void AddTag(const BRepGraph_NodeId theNode, const TCollection_AsciiString& theTag)
  {
    NCollection_FlatMap<TCollection_AsciiString>* aNodeTagsPtr = myTags.ChangeSeek(theNode);
    if (aNodeTagsPtr == nullptr)
    {
      myTags.Bind(theNode, NCollection_FlatMap<TCollection_AsciiString>());
      aNodeTagsPtr = myTags.ChangeSeek(theNode);
    }
    if (aNodeTagsPtr->Add(theTag))
    {
      touch();
    }
  }

  void RemoveTag(const BRepGraph_NodeId theNode, const TCollection_AsciiString& theTag)
  {
    NCollection_FlatMap<TCollection_AsciiString>* aNodeTags = myTags.ChangeSeek(theNode);
    if (aNodeTags == nullptr)
    {
      return;
    }
    const bool aRemoved = aNodeTags->Remove(theTag);
    if (aNodeTags->IsEmpty())
    {
      myTags.UnBind(theNode);
    }
    if (aRemoved)
    {
      touch();
    }
  }

  bool HasTag(const BRepGraph_NodeId theNode, const TCollection_AsciiString& theTag) const
  {
    const NCollection_FlatMap<TCollection_AsciiString>* aNodeTags = myTags.Seek(theNode);
    return aNodeTags != nullptr && aNodeTags->Seek(theTag) != nullptr;
  }

  void CollectTags(const BRepGraph_NodeId                      theNode,
                   NCollection_LinearVector<occtl_tag_view_t>& theTags) const
  {
    const NCollection_FlatMap<TCollection_AsciiString>* aNodeTags = myTags.Seek(theNode);
    if (aNodeTags == nullptr)
    {
      return;
    }
    for (NCollection_FlatMap<TCollection_AsciiString>::Iterator anIter(*aNodeTags); anIter.More();
         anIter.Next())
    {
      const TCollection_AsciiString& aTag = anIter.Key();
      occtl_tag_view_t               aView;
      aView.tag     = aTag.ToCString();
      aView.tag_len = asciiLength(aTag);
      theTags.Append(aView);
    }
  }

  void CollectActive(const BRepGraph&                            theGraph,
                     const TCollection_AsciiString* const        theFilter,
                     NCollection_LinearVector<BRepGraph_NodeId>& theNodes) const
  {
    for (NCollection_DataMap<BRepGraph_NodeId,
                             NCollection_FlatMap<TCollection_AsciiString>>::Iterator anIter(myTags);
         anIter.More();
         anIter.Next())
    {
      if (!isActiveNode(theGraph, anIter.Key()) || anIter.Value().IsEmpty())
      {
        continue;
      }
      if (theFilter == nullptr || anIter.Value().Seek(*theFilter) != nullptr)
      {
        theNodes.Append(anIter.Key());
      }
    }
  }

  void CopyTo(const BRepGraph&     theSource,
              BRepGraph&           theTarget,
              OcctL_Topo_TagLayer& theTargetLayer) const
  {
    for (NCollection_DataMap<BRepGraph_NodeId,
                             NCollection_FlatMap<TCollection_AsciiString>>::Iterator anIter(myTags);
         anIter.More();
         anIter.Next())
    {
      const BRepGraph_UID    aUid        = theSource.UIDs().Of(anIter.Key());
      const BRepGraph_NodeId aTargetNode = theTarget.UIDs().NodeIdFrom(aUid);
      if (!aTargetNode.IsValid())
      {
        continue;
      }
      for (NCollection_FlatMap<TCollection_AsciiString>::Iterator aTagIter(anIter.Value());
           aTagIter.More();
           aTagIter.Next())
      {
        theTargetLayer.AddTag(aTargetNode, aTagIter.Key());
      }
    }
  }

  void OnNodeRemoved(const BRepGraph_NodeId theNode) noexcept override
  {
    OnNodeReplaced(theNode, BRepGraph_NodeId());
  }

  void OnNodeReplaced(const BRepGraph_NodeId theNode,
                      const BRepGraph_NodeId theReplacement) noexcept override
  {
    const NCollection_FlatMap<TCollection_AsciiString>* anOldTags = myTags.Seek(theNode);
    if (anOldTags == nullptr)
    {
      return;
    }
    if (theReplacement.IsValid())
    {
      NCollection_FlatMap<TCollection_AsciiString>* aReplacementTagsPtr =
        myTags.ChangeSeek(theReplacement);
      if (aReplacementTagsPtr == nullptr)
      {
        myTags.Bind(theReplacement, NCollection_FlatMap<TCollection_AsciiString>());
        aReplacementTagsPtr = myTags.ChangeSeek(theReplacement);
      }
      for (NCollection_FlatMap<TCollection_AsciiString>::Iterator anIter(*anOldTags); anIter.More();
           anIter.Next())
      {
        aReplacementTagsPtr->Add(anIter.Key());
      }
    }
    myTags.UnBind(theNode);
    touch();
  }

  void OnCompact(
    const NCollection_DataMap<BRepGraph_NodeId, BRepGraph_NodeId>& theRemapMap) noexcept override
  {
    bool hasTopologyTags = false;
    NCollection_DataMap<BRepGraph_NodeId, NCollection_FlatMap<TCollection_AsciiString>> aRemapped;
    for (NCollection_DataMap<BRepGraph_NodeId,
                             NCollection_FlatMap<TCollection_AsciiString>>::Iterator anIter(myTags);
         anIter.More();
         anIter.Next())
    {
      const BRepGraph_NodeId& anOldId = anIter.Key();
      if (BRepGraph_NodeId::IsTopologyKind(anOldId.NodeKind))
      {
        hasTopologyTags                = true;
        const BRepGraph_NodeId* aNewId = theRemapMap.Seek(anOldId);
        if (aNewId != nullptr)
        {
          aRemapped.Bind(*aNewId, anIter.Value());
        }
      }
      else
      {
        aRemapped.Bind(anOldId, anIter.Value());
      }
    }
    myTags = std::move(aRemapped);
    if (hasTopologyTags)
    {
      touch();
    }
  }

  void InvalidateAll() noexcept override {}

  void Clear() noexcept override
  {
    const bool hadData = !myTags.IsEmpty();
    myTags.Clear();
    if (hadData)
    {
      touch();
    }
  }

  DEFINE_STANDARD_RTTIEXT(OcctL_Topo_TagLayer, BRepGraph_Layer)

private:
  NCollection_DataMap<BRepGraph_NodeId, NCollection_FlatMap<TCollection_AsciiString>> myTags;
};

struct JointRecord
{
  uint64_t           Id   = 0;
  occtl_joint_kind_t Kind = OCCTL_JOINT_RIGID;
  BRepGraph_NodeId   NodeA;
  BRepGraph_NodeId   NodeB;
  occtl_transform_t  FrameA      = {{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0}};
  occtl_transform_t  FrameB      = {{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0}};
  bool               HasLimitMin = false;
  double             LimitMin    = 0.0;
  bool               HasLimitMax = false;
  double             LimitMax    = 0.0;
  occtl_uid_t        MetadataUid = OCCTL_UID_INVALID;
};

class OcctL_Topo_JointLayer : public BRepGraph_Layer
{
public:
  static const Standard_GUID& GetID()
  {
    static const Standard_GUID THE_ID("b190c829-2ea0-4d01-8411-7b96f04bb6f9");
    return THE_ID;
  }

  const Standard_GUID& ID() const override { return GetID(); }

  const TCollection_AsciiString& Name() const override
  {
    static const TCollection_AsciiString THE_NAME("OCCT-Light Joints");
    return THE_NAME;
  }

  uint64_t CreateJoint(const JointRecord& theRecord)
  {
    JointRecord aRecord = theRecord;
    aRecord.Id          = myNextId++;
    myJoints.Bound(aRecord.Id, aRecord);
    touch();
    return aRecord.Id;
  }

  void AddCopyPreservingId(const JointRecord& theRecord)
  {
    myJoints.Bound(theRecord.Id, theRecord);
    myNextId = std::max(myNextId, theRecord.Id + 1);
    touch();
  }

  const JointRecord* FindJoint(const uint64_t theId) const { return myJoints.Seek(theId); }

  void RemoveJoint(const uint64_t theId)
  {
    if (myJoints.UnBind(theId))
    {
      touch();
    }
  }

  void Collect(const BRepGraph_NodeId                      theFilter,
               NCollection_LinearVector<occtl_joint_id_t>& theOut) const
  {
    for (NCollection_FlatDataMap<uint64_t, JointRecord>::Iterator anIter(myJoints); anIter.More();
         anIter.Next())
    {
      const JointRecord& aRecord = anIter.Value();
      if (!theFilter.IsValid() || aRecord.NodeA == theFilter || aRecord.NodeB == theFilter)
      {
        theOut.Append({aRecord.Id});
      }
    }
  }

  void CopyTo(const BRepGraph&       theSource,
              BRepGraph&             theTarget,
              OcctL_Topo_JointLayer& theTargetLayer) const
  {
    for (NCollection_FlatDataMap<uint64_t, JointRecord>::Iterator anIter(myJoints); anIter.More();
         anIter.Next())
    {
      JointRecord         aRecord = anIter.Value();
      const BRepGraph_UID aUidA   = theSource.UIDs().Of(aRecord.NodeA);
      const BRepGraph_UID aUidB   = theSource.UIDs().Of(aRecord.NodeB);
      aRecord.NodeA               = theTarget.UIDs().NodeIdFrom(aUidA);
      aRecord.NodeB               = theTarget.UIDs().NodeIdFrom(aUidB);
      if (aRecord.NodeA.IsValid() && aRecord.NodeB.IsValid())
      {
        theTargetLayer.AddCopyPreservingId(aRecord);
      }
    }
  }

  void OnNodeRemoved(const BRepGraph_NodeId theNode) noexcept override
  {
    OnNodeReplaced(theNode, BRepGraph_NodeId());
  }

  void OnNodeReplaced(const BRepGraph_NodeId theNode,
                      const BRepGraph_NodeId theReplacement) noexcept override
  {
    bool                               didChange = false;
    NCollection_LinearVector<uint64_t> aRemoveIds;
    for (NCollection_FlatDataMap<uint64_t, JointRecord>::Iterator anIter(myJoints); anIter.More();
         anIter.Next())
    {
      JointRecord& aRecord = anIter.ChangeValue();
      if (aRecord.NodeA != theNode && aRecord.NodeB != theNode)
      {
        continue;
      }

      if (!theReplacement.IsValid())
      {
        aRemoveIds.Append(anIter.Key());
        didChange = true;
        continue;
      }

      if (aRecord.NodeA == theNode)
      {
        aRecord.NodeA = theReplacement;
      }
      if (aRecord.NodeB == theNode)
      {
        aRecord.NodeB = theReplacement;
      }
      didChange = true;
    }

    for (const uint64_t aRemoveId : aRemoveIds)
    {
      myJoints.UnBind(aRemoveId);
    }

    if (didChange)
    {
      touch();
    }
  }

  void OnCompact(
    const NCollection_DataMap<BRepGraph_NodeId, BRepGraph_NodeId>& theRemapMap) noexcept override
  {
    bool                               didChange = false;
    NCollection_LinearVector<uint64_t> aRemoveIds;
    for (NCollection_FlatDataMap<uint64_t, JointRecord>::Iterator anIter(myJoints); anIter.More();
         anIter.Next())
    {
      JointRecord&     aRecord = anIter.ChangeValue();
      BRepGraph_NodeId aNodeA  = aRecord.NodeA;
      BRepGraph_NodeId aNodeB  = aRecord.NodeB;

      if (BRepGraph_NodeId::IsTopologyKind(aNodeA.NodeKind))
      {
        const BRepGraph_NodeId* aRemapped = theRemapMap.Seek(aNodeA);
        if (aRemapped == nullptr)
        {
          aRemoveIds.Append(anIter.Key());
          didChange = true;
          continue;
        }
        aNodeA = *aRemapped;
      }

      if (BRepGraph_NodeId::IsTopologyKind(aNodeB.NodeKind))
      {
        const BRepGraph_NodeId* aRemapped = theRemapMap.Seek(aNodeB);
        if (aRemapped == nullptr)
        {
          aRemoveIds.Append(anIter.Key());
          didChange = true;
          continue;
        }
        aNodeB = *aRemapped;
      }

      if (aNodeA != aRecord.NodeA || aNodeB != aRecord.NodeB)
      {
        aRecord.NodeA = aNodeA;
        aRecord.NodeB = aNodeB;
        didChange     = true;
      }
    }

    for (const uint64_t aRemoveId : aRemoveIds)
    {
      myJoints.UnBind(aRemoveId);
    }

    if (didChange)
    {
      touch();
    }
  }

  void InvalidateAll() noexcept override {}

  void Clear() noexcept override
  {
    const bool hadData = !myJoints.IsEmpty();
    myJoints.Clear();
    myNextId = 1;
    if (hadData)
    {
      touch();
    }
  }

  DEFINE_STANDARD_RTTIEXT(OcctL_Topo_JointLayer, BRepGraph_Layer)

private:
  NCollection_FlatDataMap<uint64_t, JointRecord> myJoints;
  uint64_t                                       myNextId = 1;
};

IMPLEMENT_STANDARD_RTTIEXT(OcctL_Topo_ColorLayer, BRepGraph_Layer)
IMPLEMENT_STANDARD_RTTIEXT(OcctL_Topo_NameLayer, BRepGraph_Layer)
IMPLEMENT_STANDARD_RTTIEXT(OcctL_Topo_MaterialLayer, BRepGraph_Layer)
IMPLEMENT_STANDARD_RTTIEXT(OcctL_Topo_UnitsLayer, BRepGraph_Layer)
IMPLEMENT_STANDARD_RTTIEXT(OcctL_Topo_MetadataLayer, BRepGraph_Layer)
IMPLEMENT_STANDARD_RTTIEXT(OcctL_Topo_TagLayer, BRepGraph_Layer)
IMPLEMENT_STANDARD_RTTIEXT(OcctL_Topo_JointLayer, BRepGraph_Layer)

occ::handle<OcctL_Topo_ColorLayer> findColorLayer(BRepGraph& theGraph)
{
  return theGraph.LayerRegistry().FindLayer<OcctL_Topo_ColorLayer>();
}

occ::handle<OcctL_Topo_ColorLayer> findColorLayer(const BRepGraph& theGraph)
{
  return occ::down_cast<OcctL_Topo_ColorLayer>(
    theGraph.LayerRegistry().FindLayer(OcctL_Topo_ColorLayer::GetID()));
}

occ::handle<OcctL_Topo_NameLayer> findNameLayer(BRepGraph& theGraph)
{
  return theGraph.LayerRegistry().FindLayer<OcctL_Topo_NameLayer>();
}

occ::handle<OcctL_Topo_NameLayer> findNameLayer(const BRepGraph& theGraph)
{
  return occ::down_cast<OcctL_Topo_NameLayer>(
    theGraph.LayerRegistry().FindLayer(OcctL_Topo_NameLayer::GetID()));
}

occ::handle<OcctL_Topo_MaterialLayer> findMaterialLayer(BRepGraph& theGraph)
{
  return theGraph.LayerRegistry().FindLayer<OcctL_Topo_MaterialLayer>();
}

occ::handle<OcctL_Topo_MaterialLayer> findMaterialLayer(const BRepGraph& theGraph)
{
  return occ::down_cast<OcctL_Topo_MaterialLayer>(
    theGraph.LayerRegistry().FindLayer(OcctL_Topo_MaterialLayer::GetID()));
}

occ::handle<OcctL_Topo_UnitsLayer> findUnitsLayer(BRepGraph& theGraph)
{
  return theGraph.LayerRegistry().FindLayer<OcctL_Topo_UnitsLayer>();
}

occ::handle<OcctL_Topo_UnitsLayer> findUnitsLayer(const BRepGraph& theGraph)
{
  return occ::down_cast<OcctL_Topo_UnitsLayer>(
    theGraph.LayerRegistry().FindLayer(OcctL_Topo_UnitsLayer::GetID()));
}

occ::handle<OcctL_Topo_MetadataLayer> findMetadataLayer(BRepGraph& theGraph)
{
  return theGraph.LayerRegistry().FindLayer<OcctL_Topo_MetadataLayer>();
}

occ::handle<OcctL_Topo_MetadataLayer> findMetadataLayer(const BRepGraph& theGraph)
{
  return occ::down_cast<OcctL_Topo_MetadataLayer>(
    theGraph.LayerRegistry().FindLayer(OcctL_Topo_MetadataLayer::GetID()));
}

occ::handle<OcctL_Topo_TagLayer> findTagLayer(BRepGraph& theGraph)
{
  return theGraph.LayerRegistry().FindLayer<OcctL_Topo_TagLayer>();
}

occ::handle<OcctL_Topo_TagLayer> findTagLayer(const BRepGraph& theGraph)
{
  return occ::down_cast<OcctL_Topo_TagLayer>(
    theGraph.LayerRegistry().FindLayer(OcctL_Topo_TagLayer::GetID()));
}

occ::handle<OcctL_Topo_JointLayer> findJointLayer(BRepGraph& theGraph)
{
  return theGraph.LayerRegistry().FindLayer<OcctL_Topo_JointLayer>();
}

occ::handle<OcctL_Topo_JointLayer> findJointLayer(const BRepGraph& theGraph)
{
  return occ::down_cast<OcctL_Topo_JointLayer>(
    theGraph.LayerRegistry().FindLayer(OcctL_Topo_JointLayer::GetID()));
}

bool isFlag01(const int32_t theFlag)
{
  return theFlag == 0 || theFlag == 1;
}

bool isFiniteColor(const occtl_color_rgba_t& theColor)
{
  return IsFiniteValue(theColor.r) && IsFiniteValue(theColor.g) && IsFiniteValue(theColor.b)
         && IsFiniteValue(theColor.a);
}

bool isFiniteTransform(const occtl_transform_t& theTransform)
{
  for (int anI = 0; anI < 12; ++anI)
  {
    if (!IsFiniteValue(theTransform.m[anI]))
    {
      return false;
    }
  }
  return true;
}

bool isActiveNode(const BRepGraph& theGraph, const BRepGraph_NodeId theNode)
{
  return theNode.IsValid() && !theGraph.Topo().Gen().IsRemoved(theNode);
}

bool isKnownJointKind(const occtl_joint_kind_t theKind)
{
  return theKind == OCCTL_JOINT_RIGID || theKind == OCCTL_JOINT_REVOLUTE
         || theKind == OCCTL_JOINT_LINEAR || theKind == OCCTL_JOINT_CYLINDRICAL
         || theKind == OCCTL_JOINT_BALL;
}

occtl_status_t materialRecordFromInfo(const occtl_material_info_t& theInfo,
                                      MaterialRecord&              theRecord)
{
  if (theInfo.struct_version != OCCTL_MATERIAL_INFO_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "info->struct_version is not OCCTL_MATERIAL_INFO_VERSION_1");
    return OCCTL_VERSION_MISMATCH;
  }

  if (theInfo.p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }

  if (theInfo.name_len > 0 && theInfo.name == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "info->name is NULL with non-zero name_len");
    return OCCTL_INVALID_ARGUMENT;
  }

  if (!isFlag01(theInfo.has_density) || !isFlag01(theInfo.has_diffuse_color))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "material presence flags must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }

  if (theInfo.has_density != 0 && (!IsFiniteValue(theInfo.density) || theInfo.density <= 0.0))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "material density must be finite and positive");
    return OCCTL_INVALID_ARGUMENT;
  }

  if (theInfo.has_diffuse_color != 0 && !isFiniteColor(theInfo.diffuse_color))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "material diffuse color channels must be finite");
    return OCCTL_INVALID_ARGUMENT;
  }

  theRecord.Name = theInfo.name_len > 0
                     ? TCollection_AsciiString(theInfo.name, static_cast<int>(theInfo.name_len))
                     : TCollection_AsciiString();
  theRecord.HasDensity      = theInfo.has_density != 0;
  theRecord.Density         = theInfo.density;
  theRecord.HasDiffuseColor = theInfo.has_diffuse_color != 0;
  theRecord.DiffuseColor    = Quantity_ColorRGBA(theInfo.diffuse_color.r,
                                                 theInfo.diffuse_color.g,
                                                 theInfo.diffuse_color.b,
                                                 theInfo.diffuse_color.a);
  theRecord.MetadataUid     = theInfo.metadata_uid;
  return OCCTL_OK;
}

void fillMaterialInfo(const MaterialRecord&  theRecord,
                      const char* const      theName,
                      occtl_material_info_t& theInfo)
{
  theInfo                   = OCCTL_MATERIAL_INFO_INIT;
  theInfo.name              = theName;
  theInfo.name_len          = asciiLength(theRecord.Name);
  theInfo.has_density       = theRecord.HasDensity ? 1 : 0;
  theInfo.density           = theRecord.Density;
  theInfo.has_diffuse_color = theRecord.HasDiffuseColor ? 1 : 0;
  theInfo.diffuse_color.r   = static_cast<float>(theRecord.DiffuseColor.GetRGB().Red());
  theInfo.diffuse_color.g   = static_cast<float>(theRecord.DiffuseColor.GetRGB().Green());
  theInfo.diffuse_color.b   = static_cast<float>(theRecord.DiffuseColor.GetRGB().Blue());
  theInfo.diffuse_color.a   = static_cast<float>(theRecord.DiffuseColor.Alpha());
  theInfo.metadata_uid      = theRecord.MetadataUid;
}

occtl_status_t jointRecordFromInfo(const BRepGraph&          theGraph,
                                   const occtl_joint_info_t& theInfo,
                                   JointRecord&              theRecord)
{
  if (theInfo.struct_version != OCCTL_JOINT_INFO_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "info->struct_version is not OCCTL_JOINT_INFO_VERSION_1");
    return OCCTL_VERSION_MISMATCH;
  }

  if (theInfo.p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }

  if (!isKnownJointKind(theInfo.kind))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "info->kind is not a known joint kind");
    return OCCTL_INVALID_ARGUMENT;
  }

  if (!isFlag01(theInfo.has_limit_min) || !isFlag01(theInfo.has_limit_max))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "joint limit flags must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }

  if (!isFiniteTransform(theInfo.frame_a) || !isFiniteTransform(theInfo.frame_b))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "joint frames must contain only finite values");
    return OCCTL_INVALID_ARGUMENT;
  }

  if ((theInfo.has_limit_min != 0 && !IsFiniteValue(theInfo.limit_min))
      || (theInfo.has_limit_max != 0 && !IsFiniteValue(theInfo.limit_max)))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "joint limits must be finite when present");
    return OCCTL_INVALID_ARGUMENT;
  }

  if (theInfo.has_limit_min != 0 && theInfo.has_limit_max != 0
      && theInfo.limit_min > theInfo.limit_max)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "joint lower limit is greater than upper limit");
    return OCCTL_INVALID_ARGUMENT;
  }

  const BRepGraph_NodeId aNodeA = OcctL::Topo::UnpackNodeId(theInfo.node_a);
  const BRepGraph_NodeId aNodeB = OcctL::Topo::UnpackNodeId(theInfo.node_b);
  if (!isActiveNode(theGraph, aNodeA) || !isActiveNode(theGraph, aNodeB))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                           "joint endpoint node is invalid or removed");
    return OCCTL_NOT_FOUND;
  }

  theRecord.Kind        = theInfo.kind;
  theRecord.NodeA       = aNodeA;
  theRecord.NodeB       = aNodeB;
  theRecord.FrameA      = theInfo.frame_a;
  theRecord.FrameB      = theInfo.frame_b;
  theRecord.HasLimitMin = theInfo.has_limit_min != 0;
  theRecord.LimitMin    = theInfo.limit_min;
  theRecord.HasLimitMax = theInfo.has_limit_max != 0;
  theRecord.LimitMax    = theInfo.limit_max;
  theRecord.MetadataUid = theInfo.metadata_uid;
  return OCCTL_OK;
}

void fillJointInfo(const JointRecord& theRecord, occtl_joint_info_t& theInfo)
{
  theInfo               = OCCTL_JOINT_INFO_INIT;
  theInfo.id.bits       = theRecord.Id;
  theInfo.kind          = theRecord.Kind;
  theInfo.node_a        = OcctL::Topo::PackNodeId(theRecord.NodeA);
  theInfo.node_b        = OcctL::Topo::PackNodeId(theRecord.NodeB);
  theInfo.frame_a       = theRecord.FrameA;
  theInfo.frame_b       = theRecord.FrameB;
  theInfo.has_limit_min = theRecord.HasLimitMin ? 1 : 0;
  theInfo.limit_min     = theRecord.LimitMin;
  theInfo.has_limit_max = theRecord.HasLimitMax ? 1 : 0;
  theInfo.limit_max     = theRecord.LimitMax;
  theInfo.metadata_uid  = theRecord.MetadataUid;
}

occtl_status_t fillNodeBuffer(const NCollection_LinearVector<BRepGraph_NodeId>& theNodes,
                              occtl_node_id_t* const                            theOutNodes,
                              const size_t                                      theCap,
                              size_t* const                                     theOutCount,
                              const char* const                                 theContext)
{
  *theOutCount = theNodes.Size();
  if (theOutNodes == nullptr)
  {
    return OCCTL_OK;
  }
  if (theCap < theNodes.Size())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_BUFFER_TOO_SMALL,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": output buffer is too small"));
    return OCCTL_BUFFER_TOO_SMALL;
  }
  for (size_t anIndex = 0; anIndex < theNodes.Size(); ++anIndex)
  {
    theOutNodes[anIndex] = OcctL::Topo::PackNodeId(theNodes.Value(anIndex));
  }
  return OCCTL_OK;
}

} // namespace

namespace OcctL::Topo
{

void EnsureBuiltinLayers(BRepGraph& theGraph)
{
  if (findColorLayer(theGraph).IsNull())
  {
    theGraph.LayerRegistry().RegisterLayer(new OcctL_Topo_ColorLayer());
  }
  if (findNameLayer(theGraph).IsNull())
  {
    theGraph.LayerRegistry().RegisterLayer(new OcctL_Topo_NameLayer());
  }
  if (findMaterialLayer(theGraph).IsNull())
  {
    theGraph.LayerRegistry().RegisterLayer(new OcctL_Topo_MaterialLayer());
  }
  if (findUnitsLayer(theGraph).IsNull())
  {
    theGraph.LayerRegistry().RegisterLayer(new OcctL_Topo_UnitsLayer());
  }
  if (findMetadataLayer(theGraph).IsNull())
  {
    theGraph.LayerRegistry().RegisterLayer(new OcctL_Topo_MetadataLayer());
  }
  if (findTagLayer(theGraph).IsNull())
  {
    theGraph.LayerRegistry().RegisterLayer(new OcctL_Topo_TagLayer());
  }
  if (findJointLayer(theGraph).IsNull())
  {
    theGraph.LayerRegistry().RegisterLayer(new OcctL_Topo_JointLayer());
  }
}

void CopyBuiltinLayers(const BRepGraph& theSource, BRepGraph& theTarget)
{
  EnsureBuiltinLayers(theTarget);

  const occ::handle<OcctL_Topo_ColorLayer> aSourceColor = findColorLayer(theSource);
  occ::handle<OcctL_Topo_ColorLayer>       aTargetColor = findColorLayer(theTarget);
  if (!aSourceColor.IsNull() && !aTargetColor.IsNull())
  {
    aSourceColor->CopyTo(theSource, theTarget, *aTargetColor);
  }

  const occ::handle<OcctL_Topo_NameLayer> aSourceName = findNameLayer(theSource);
  occ::handle<OcctL_Topo_NameLayer>       aTargetName = findNameLayer(theTarget);
  if (!aSourceName.IsNull() && !aTargetName.IsNull())
  {
    aSourceName->CopyTo(theSource, theTarget, *aTargetName);
  }

  const occ::handle<OcctL_Topo_MaterialLayer> aSourceMaterial = findMaterialLayer(theSource);
  occ::handle<OcctL_Topo_MaterialLayer>       aTargetMaterial = findMaterialLayer(theTarget);
  if (!aSourceMaterial.IsNull() && !aTargetMaterial.IsNull())
  {
    aSourceMaterial->CopyTo(theSource, theTarget, *aTargetMaterial);
  }

  const occ::handle<OcctL_Topo_UnitsLayer> aSourceUnits = findUnitsLayer(theSource);
  occ::handle<OcctL_Topo_UnitsLayer>       aTargetUnits = findUnitsLayer(theTarget);
  if (!aSourceUnits.IsNull() && !aTargetUnits.IsNull())
  {
    aSourceUnits->CopyTo(*aTargetUnits);
  }

  const occ::handle<OcctL_Topo_MetadataLayer> aSourceMetadata = findMetadataLayer(theSource);
  occ::handle<OcctL_Topo_MetadataLayer>       aTargetMetadata = findMetadataLayer(theTarget);
  if (!aSourceMetadata.IsNull() && !aTargetMetadata.IsNull())
  {
    aSourceMetadata->CopyTo(theSource, theTarget, *aTargetMetadata);
  }

  const occ::handle<OcctL_Topo_TagLayer> aSourceTag = findTagLayer(theSource);
  occ::handle<OcctL_Topo_TagLayer>       aTargetTag = findTagLayer(theTarget);
  if (!aSourceTag.IsNull() && !aTargetTag.IsNull())
  {
    aSourceTag->CopyTo(theSource, theTarget, *aTargetTag);
  }

  const occ::handle<OcctL_Topo_JointLayer> aSourceJoint = findJointLayer(theSource);
  occ::handle<OcctL_Topo_JointLayer>       aTargetJoint = findJointLayer(theTarget);
  if (!aSourceJoint.IsNull() && !aTargetJoint.IsNull())
  {
    aSourceJoint->CopyTo(theSource, theTarget, *aTargetJoint);
  }
}

bool FindBuiltinColor(const BRepGraph&       theGraph,
                      const BRepGraph_NodeId theNode,
                      Quantity_ColorRGBA&    theColor)
{
  const occ::handle<OcctL_Topo_ColorLayer> aLayer = findColorLayer(theGraph);
  return !aLayer.IsNull() && aLayer->FindColor(theNode, theColor);
}

bool FindBuiltinName(const BRepGraph&         theGraph,
                     const BRepGraph_NodeId   theNode,
                     TCollection_AsciiString& theName)
{
  const occ::handle<OcctL_Topo_NameLayer> aLayer = findNameLayer(theGraph);
  if (aLayer.IsNull())
  {
    return false;
  }
  const TCollection_AsciiString* aName = aLayer->FindName(theNode);
  if (aName == nullptr)
  {
    return false;
  }
  theName = *aName;
  return true;
}

bool FindBuiltinMetadata(const BRepGraph&               theGraph,
                         const BRepGraph_NodeId         theNode,
                         const TCollection_AsciiString& theKey,
                         TCollection_AsciiString&       theValue)
{
  const occ::handle<OcctL_Topo_MetadataLayer> aLayer = findMetadataLayer(theGraph);
  if (aLayer.IsNull())
  {
    return false;
  }
  const TCollection_AsciiString* aValue = aLayer->FindValue(theNode, theKey);
  if (aValue == nullptr)
  {
    return false;
  }
  theValue = *aValue;
  return true;
}

bool HasBuiltinTag(const BRepGraph&               theGraph,
                   const BRepGraph_NodeId         theNode,
                   const TCollection_AsciiString& theTag)
{
  const occ::handle<OcctL_Topo_TagLayer> aLayer = findTagLayer(theGraph);
  return !aLayer.IsNull() && aLayer->HasTag(theNode, theTag);
}

} // namespace OcctL::Topo

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_color_set(occtl_graph_t* const     theGraph,
                                                          const occtl_node_id_t    theTarget,
                                                          const occtl_color_rgba_t theColor)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }
    if (!isFiniteColor(theColor))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "color channels must be finite");
      return OCCTL_INVALID_ARGUMENT;
    }

    occ::handle<OcctL_Topo_ColorLayer> aLayer = findColorLayer(theGraph->graph);
    if (aLayer.IsNull())
    {
      OcctL::Topo::EnsureBuiltinLayers(theGraph->graph);
      aLayer = findColorLayer(theGraph->graph);
    }
    aLayer->SetColor(aNodeId, Quantity_ColorRGBA(theColor.r, theColor.g, theColor.b, theColor.a));
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_color_get(const occtl_graph_t* const theGraph,
                                                          const occtl_node_id_t      theTarget,
                                                          occtl_color_rgba_t* const  theOutColor)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutColor == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutColor is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    Quantity_ColorRGBA                       aColor;
    const occ::handle<OcctL_Topo_ColorLayer> aLayer = findColorLayer(theGraph->graph);
    if (!aLayer.IsNull() && aLayer->FindColor(aNodeId, aColor))
    {
      theOutColor->r = static_cast<float>(aColor.GetRGB().Red());
      theOutColor->g = static_cast<float>(aColor.GetRGB().Green());
      theOutColor->b = static_cast<float>(aColor.GetRGB().Blue());
      theOutColor->a = static_cast<float>(aColor.Alpha());
    }
    else
    {
      theOutColor->r = 1.0f;
      theOutColor->g = 1.0f;
      theOutColor->b = 1.0f;
      theOutColor->a = 1.0f;
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_color_unset(occtl_graph_t* const  theGraph,
                                                            const occtl_node_id_t theTarget)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    const occ::handle<OcctL_Topo_ColorLayer> aLayer = findColorLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->RemoveColor(aNodeId);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_color_entries(const occtl_graph_t* const theGraph,
                            occtl_node_id_t* const     theOutNodes,
                            occtl_color_rgba_t* const  theOutColors,
                            const size_t               theCap,
                            size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if ((theOutNodes == nullptr) != (theOutColors == nullptr))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "theOutNodes and theOutColors must both be NULL or both be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_LinearVector<BRepGraph_NodeId>   aNodes;
    NCollection_LinearVector<Quantity_ColorRGBA> aColors;
    const occ::handle<OcctL_Topo_ColorLayer>     aLayer = findColorLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->CollectActive(theGraph->graph, aNodes, aColors);
    }

    *theOutCount = aNodes.Size();
    if (theOutNodes == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < aNodes.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theCap is too small for color entries");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (size_t anIndex = 0; anIndex < aNodes.Size(); ++anIndex)
    {
      const Quantity_ColorRGBA& aColor = aColors.Value(anIndex);
      theOutNodes[anIndex]             = OcctL::Topo::PackNodeId(aNodes.Value(anIndex));
      theOutColors[anIndex].r          = static_cast<float>(aColor.GetRGB().Red());
      theOutColors[anIndex].g          = static_cast<float>(aColor.GetRGB().Green());
      theOutColors[anIndex].b          = static_cast<float>(aColor.GetRGB().Blue());
      theOutColors[anIndex].a          = static_cast<float>(aColor.Alpha());
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_name_set(occtl_graph_t* const  theGraph,
                                                         const occtl_node_id_t theTarget,
                                                         const char* const     theName,
                                                         const size_t          theNameLen)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theNameLen > 0 && theName == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theName is NULL with non-zero theNameLen");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    if (theNameLen == 0)
    {
      const occ::handle<OcctL_Topo_NameLayer> aLayer = findNameLayer(theGraph->graph);
      if (!aLayer.IsNull())
      {
        aLayer->RemoveName(aNodeId);
      }
    }
    else
    {
      occ::handle<OcctL_Topo_NameLayer> aLayer = findNameLayer(theGraph->graph);
      if (aLayer.IsNull())
      {
        OcctL::Topo::EnsureBuiltinLayers(theGraph->graph);
        aLayer = findNameLayer(theGraph->graph);
      }
      aLayer->SetName(aNodeId, TCollection_AsciiString(theName, static_cast<int>(theNameLen)));
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_name_get(const occtl_graph_t* const theGraph,
                                                         const occtl_node_id_t      theTarget,
                                                         char* const                theBuf,
                                                         const size_t               theBufSize,
                                                         size_t* const              theOutRequired)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutRequired == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutRequired is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    const occ::handle<OcctL_Topo_NameLayer> aLayer = findNameLayer(theGraph->graph);
    const TCollection_AsciiString* aName = !aLayer.IsNull() ? aLayer->FindName(aNodeId) : nullptr;
    const TCollection_AsciiString  anEmpty;
    const TCollection_AsciiString& aNameRef = aName != nullptr ? *aName : anEmpty;

    const size_t aRequired = asciiLength(aNameRef) + 1;
    *theOutRequired        = aRequired;

    if (theBuf == nullptr)
    {
      return OCCTL_OK;
    }

    if (theBufSize < aRequired)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theBufSize is too small for the name");
      return OCCTL_BUFFER_TOO_SMALL;
    }

    std::memcpy(theBuf, aNameRef.ToCString(), asciiLength(aNameRef));
    theBuf[asciiLength(aNameRef)] = '\0';
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_material_set(occtl_graph_t* const               theGraph,
                           const occtl_node_id_t              theTarget,
                           const occtl_material_info_t* const theInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph or theInfo is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    MaterialRecord aRecord;
    if (const occtl_status_t aStatus = materialRecordFromInfo(*theInfo, aRecord))
    {
      return aStatus;
    }

    occ::handle<OcctL_Topo_MaterialLayer> aLayer = findMaterialLayer(theGraph->graph);
    if (aLayer.IsNull())
    {
      OcctL::Topo::EnsureBuiltinLayers(theGraph->graph);
      aLayer = findMaterialLayer(theGraph->graph);
    }
    aLayer->SetMaterial(aNodeId, aRecord);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_name_nodes(const occtl_graph_t* const theGraph,
                                                           occtl_node_id_t* const     theOutNodes,
                                                           const size_t               theCap,
                                                           size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_LinearVector<BRepGraph_NodeId> aNodes;
    const occ::handle<OcctL_Topo_NameLayer>    aLayer = findNameLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->CollectActive(theGraph->graph, aNodes);
    }
    return fillNodeBuffer(aNodes, theOutNodes, theCap, theOutCount, "occtl_graph_name_nodes");
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_material_get(const occtl_graph_t* const   theGraph,
                           const occtl_node_id_t        theTarget,
                           occtl_material_info_t* const theOutInfo,
                           char* const                  theNameBuf,
                           const size_t                 theNameBufSize,
                           size_t* const                theOutNameRequired)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutInfo == nullptr || theOutNameRequired == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph, theOutInfo, or theOutNameRequired is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    const occ::handle<OcctL_Topo_MaterialLayer> aLayer = findMaterialLayer(theGraph->graph);
    const MaterialRecord* aRecord = !aLayer.IsNull() ? aLayer->FindMaterial(aNodeId) : nullptr;
    if (aRecord == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "material is not set");
      return OCCTL_NOT_FOUND;
    }

    const size_t aRequired = asciiLength(aRecord->Name) + 1;
    *theOutNameRequired    = aRequired;
    fillMaterialInfo(*aRecord, nullptr, *theOutInfo);

    if (theNameBuf == nullptr)
    {
      return OCCTL_OK;
    }

    if (theNameBufSize < aRequired)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theNameBufSize is too small for the material name");
      return OCCTL_BUFFER_TOO_SMALL;
    }

    std::memcpy(theNameBuf, aRecord->Name.ToCString(), asciiLength(aRecord->Name));
    theNameBuf[asciiLength(aRecord->Name)] = '\0';
    theOutInfo->name                       = theNameBuf;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_material_unset(occtl_graph_t* const  theGraph,
                                                               const occtl_node_id_t theTarget)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    const occ::handle<OcctL_Topo_MaterialLayer> aLayer = findMaterialLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->RemoveMaterial(aNodeId);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_material_nodes(const occtl_graph_t* const theGraph,
                                                               occtl_node_id_t* const theOutNodes,
                                                               const size_t           theCap,
                                                               size_t* const          theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_LinearVector<BRepGraph_NodeId>  aNodes;
    const occ::handle<OcctL_Topo_MaterialLayer> aLayer = findMaterialLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->CollectActive(theGraph->graph, aNodes);
    }
    return fillNodeBuffer(aNodes, theOutNodes, theCap, theOutCount, "occtl_graph_material_nodes");
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_units_set(occtl_graph_t* const theGraph,
                                                          const double         theLengthUnitToMeter,
                                                          const char* const    theName,
                                                          const size_t         theNameLen)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (!IsFiniteValue(theLengthUnitToMeter) || theLengthUnitToMeter <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theLengthUnitToMeter must be finite and positive");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theNameLen > 0 && theName == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theName is NULL with non-zero theNameLen");
      return OCCTL_INVALID_ARGUMENT;
    }

    occ::handle<OcctL_Topo_UnitsLayer> aLayer = findUnitsLayer(theGraph->graph);
    if (aLayer.IsNull())
    {
      OcctL::Topo::EnsureBuiltinLayers(theGraph->graph);
      aLayer = findUnitsLayer(theGraph->graph);
    }
    aLayer->SetUnits(theLengthUnitToMeter,
                     theNameLen > 0 ? TCollection_AsciiString(theName, static_cast<int>(theNameLen))
                                    : TCollection_AsciiString());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_units_get(const occtl_graph_t* const theGraph,
                                                          double* const theOutLengthUnitToMeter,
                                                          char* const   theNameBuf,
                                                          const size_t  theNameBufSize,
                                                          size_t* const theOutNameRequired)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutLengthUnitToMeter == nullptr || theOutNameRequired == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "theGraph, theOutLengthUnitToMeter, or theOutNameRequired is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    double                                   aLengthUnitToMeter = 1.0;
    TCollection_AsciiString                  aName              = "m";
    const occ::handle<OcctL_Topo_UnitsLayer> aLayer             = findUnitsLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      (void)aLayer->FindUnits(aLengthUnitToMeter, aName);
    }

    *theOutLengthUnitToMeter = aLengthUnitToMeter;
    const size_t aRequired   = asciiLength(aName) + 1;
    *theOutNameRequired      = aRequired;

    if (theNameBuf == nullptr)
    {
      return OCCTL_OK;
    }

    if (theNameBufSize < aRequired)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theNameBufSize is too small for the unit name");
      return OCCTL_BUFFER_TOO_SMALL;
    }

    std::memcpy(theNameBuf, aName.ToCString(), asciiLength(aName));
    theNameBuf[asciiLength(aName)] = '\0';
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_node_metadata_set(occtl_graph_t* const  theGraph,
                                                                  const occtl_node_id_t theTarget,
                                                                  const char* const     theKey,
                                                                  const size_t          theKeyLen,
                                                                  const char* const     theValue,
                                                                  const size_t          theValueLen)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theKey == nullptr || theKeyLen == 0 || (theValueLen > 0 && theValue == nullptr))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "theKey is NULL/empty, or theValue is NULL with non-zero theValueLen");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    occ::handle<OcctL_Topo_MetadataLayer> aLayer = findMetadataLayer(theGraph->graph);
    if (aLayer.IsNull())
    {
      OcctL::Topo::EnsureBuiltinLayers(theGraph->graph);
      aLayer = findMetadataLayer(theGraph->graph);
    }
    aLayer->SetValue(aNodeId,
                     TCollection_AsciiString(theKey, static_cast<int>(theKeyLen)),
                     theValueLen > 0
                       ? TCollection_AsciiString(theValue, static_cast<int>(theValueLen))
                       : TCollection_AsciiString());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_node_metadata_get(const occtl_graph_t* const theGraph,
                                const occtl_node_id_t      theTarget,
                                const char* const          theKey,
                                const size_t               theKeyLen,
                                char* const                theBuf,
                                const size_t               theBufSize,
                                size_t* const              theOutRequired)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutRequired == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutRequired is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theKey == nullptr || theKeyLen == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theKey is NULL or empty");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    const occ::handle<OcctL_Topo_MetadataLayer> aLayer = findMetadataLayer(theGraph->graph);
    const TCollection_AsciiString               aKey(theKey, static_cast<int>(theKeyLen));
    const TCollection_AsciiString*              aValue =
      !aLayer.IsNull() ? aLayer->FindValue(aNodeId, aKey) : nullptr;
    if (aValue == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "metadata key is not set");
      return OCCTL_NOT_FOUND;
    }

    const size_t aRequired = asciiLength(*aValue) + 1;
    *theOutRequired        = aRequired;

    if (theBuf == nullptr)
    {
      return OCCTL_OK;
    }

    if (theBufSize < aRequired)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theBufSize is too small for the metadata value");
      return OCCTL_BUFFER_TOO_SMALL;
    }

    std::memcpy(theBuf, aValue->ToCString(), asciiLength(*aValue));
    theBuf[asciiLength(*aValue)] = '\0';
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_node_metadata_keys(const occtl_graph_t* const       theGraph,
                                 const occtl_node_id_t            theTarget,
                                 occtl_metadata_key_view_t* const theOutKeys,
                                 const size_t                     theCap,
                                 size_t* const                    theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    NCollection_LinearVector<occtl_metadata_key_view_t> aKeys;
    const occ::handle<OcctL_Topo_MetadataLayer>         aLayer = findMetadataLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->CollectKeys(aNodeId, aKeys);
    }
    *theOutCount = aKeys.Size();

    if (theOutKeys == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < aKeys.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theCap is too small for metadata keys");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (size_t aKeyIndex = 0; aKeyIndex < aKeys.Size(); ++aKeyIndex)
    {
      theOutKeys[aKeyIndex] = aKeys.Value(aKeyIndex);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_node_metadata_unset(occtl_graph_t* const  theGraph,
                                                                    const occtl_node_id_t theTarget,
                                                                    const char* const     theKey,
                                                                    const size_t          theKeyLen)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theKey == nullptr || theKeyLen == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theKey is NULL or empty");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    const occ::handle<OcctL_Topo_MetadataLayer> aLayer = findMetadataLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->RemoveValue(aNodeId, TCollection_AsciiString(theKey, static_cast<int>(theKeyLen)));
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_node_metadata_nodes(const occtl_graph_t* const theGraph,
                                  occtl_node_id_t* const     theOutNodes,
                                  const size_t               theCap,
                                  size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_LinearVector<BRepGraph_NodeId>  aNodes;
    const occ::handle<OcctL_Topo_MetadataLayer> aLayer = findMetadataLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->CollectActive(theGraph->graph, aNodes);
    }
    return fillNodeBuffer(aNodes,
                          theOutNodes,
                          theCap,
                          theOutCount,
                          "occtl_graph_node_metadata_nodes");
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_metadata_set(occtl_graph_t* const theGraph,
                                                             const char* const    theKey,
                                                             const size_t         theKeyLen,
                                                             const char* const    theValue,
                                                             const size_t         theValueLen)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theKey == nullptr || theKeyLen == 0 || (theValueLen > 0 && theValue == nullptr))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "theKey is NULL/empty, or theValue is NULL with non-zero theValueLen");
      return OCCTL_INVALID_ARGUMENT;
    }

    occ::handle<OcctL_Topo_MetadataLayer> aLayer = findMetadataLayer(theGraph->graph);
    if (aLayer.IsNull())
    {
      OcctL::Topo::EnsureBuiltinLayers(theGraph->graph);
      aLayer = findMetadataLayer(theGraph->graph);
    }
    aLayer->SetGraphValue(TCollection_AsciiString(theKey, static_cast<int>(theKeyLen)),
                          theValueLen > 0
                            ? TCollection_AsciiString(theValue, static_cast<int>(theValueLen))
                            : TCollection_AsciiString());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_metadata_get(const occtl_graph_t* const theGraph,
                                                             const char* const          theKey,
                                                             const size_t               theKeyLen,
                                                             char* const                theBuf,
                                                             const size_t               theBufSize,
                                                             size_t* const theOutRequired)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutRequired == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutRequired is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theKey == nullptr || theKeyLen == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theKey is NULL or empty");
      return OCCTL_INVALID_ARGUMENT;
    }

    const occ::handle<OcctL_Topo_MetadataLayer> aLayer = findMetadataLayer(theGraph->graph);
    const TCollection_AsciiString               aKey(theKey, static_cast<int>(theKeyLen));
    const TCollection_AsciiString*              aValue =
      !aLayer.IsNull() ? aLayer->FindGraphValue(aKey) : nullptr;
    if (aValue == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "graph metadata key is not set");
      return OCCTL_NOT_FOUND;
    }

    const size_t aRequired = asciiLength(*aValue) + 1;
    *theOutRequired        = aRequired;

    if (theBuf == nullptr)
    {
      return OCCTL_OK;
    }

    if (theBufSize < aRequired)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theBufSize is too small for the metadata value");
      return OCCTL_BUFFER_TOO_SMALL;
    }

    std::memcpy(theBuf, aValue->ToCString(), asciiLength(*aValue));
    theBuf[asciiLength(*aValue)] = '\0';
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_metadata_keys(const occtl_graph_t* const       theGraph,
                            occtl_metadata_key_view_t* const theOutKeys,
                            const size_t                     theCap,
                            size_t* const                    theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_LinearVector<occtl_metadata_key_view_t> aKeys;
    const occ::handle<OcctL_Topo_MetadataLayer>         aLayer = findMetadataLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->CollectGraphKeys(aKeys);
    }
    *theOutCount = aKeys.Size();

    if (theOutKeys == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < aKeys.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theCap is too small for graph metadata keys");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (size_t aKeyIndex = 0; aKeyIndex < aKeys.Size(); ++aKeyIndex)
    {
      theOutKeys[aKeyIndex] = aKeys.Value(aKeyIndex);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_metadata_unset(occtl_graph_t* const theGraph,
                                                               const char* const    theKey,
                                                               const size_t         theKeyLen)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theKey == nullptr || theKeyLen == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theKey is NULL or empty");
      return OCCTL_INVALID_ARGUMENT;
    }

    const occ::handle<OcctL_Topo_MetadataLayer> aLayer = findMetadataLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->RemoveGraphValue(TCollection_AsciiString(theKey, static_cast<int>(theKeyLen)));
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_tag_add(occtl_graph_t* const  theGraph,
                                                        const occtl_node_id_t theTarget,
                                                        const char* const     theTag,
                                                        const size_t          theTagLen)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theTag == nullptr || theTagLen == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theTag is NULL or empty");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    occ::handle<OcctL_Topo_TagLayer> aLayer = findTagLayer(theGraph->graph);
    if (aLayer.IsNull())
    {
      OcctL::Topo::EnsureBuiltinLayers(theGraph->graph);
      aLayer = findTagLayer(theGraph->graph);
    }
    aLayer->AddTag(aNodeId, TCollection_AsciiString(theTag, static_cast<int>(theTagLen)));
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_tag_remove(occtl_graph_t* const  theGraph,
                                                           const occtl_node_id_t theTarget,
                                                           const char* const     theTag,
                                                           const size_t          theTagLen)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theTag == nullptr || theTagLen == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theTag is NULL or empty");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    const occ::handle<OcctL_Topo_TagLayer> aLayer = findTagLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->RemoveTag(aNodeId, TCollection_AsciiString(theTag, static_cast<int>(theTagLen)));
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_tag_has(const occtl_graph_t* const theGraph,
                                                        const occtl_node_id_t      theTarget,
                                                        const char* const          theTag,
                                                        const size_t               theTagLen,
                                                        int32_t* const             theOutHas)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutHas == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutHas is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theTag == nullptr || theTagLen == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theTag is NULL or empty");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    const occ::handle<OcctL_Topo_TagLayer> aLayer = findTagLayer(theGraph->graph);
    *theOutHas =
      (!aLayer.IsNull()
       && aLayer->HasTag(aNodeId, TCollection_AsciiString(theTag, static_cast<int>(theTagLen))))
        ? 1
        : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_tag_list(const occtl_graph_t* const theGraph,
                                                         const occtl_node_id_t      theTarget,
                                                         occtl_tag_view_t* const    theOutTags,
                                                         const size_t               theCap,
                                                         size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theTarget);
    if (!aNodeId.IsValid() || !isActiveNode(theGraph->graph, aNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theTarget is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    NCollection_LinearVector<occtl_tag_view_t> aTags;
    const occ::handle<OcctL_Topo_TagLayer>     aLayer = findTagLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->CollectTags(aNodeId, aTags);
    }
    *theOutCount = aTags.Size();

    if (theOutTags == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < aTags.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theCap is too small for tags");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (size_t aTagIndex = 0; aTagIndex < aTags.Size(); ++aTagIndex)
    {
      theOutTags[aTagIndex] = aTags.Value(aTagIndex);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_tag_nodes(const occtl_graph_t* const theGraph,
                                                          const char* const          theTag,
                                                          const size_t               theTagLen,
                                                          occtl_node_id_t* const     theOutNodes,
                                                          const size_t               theCap,
                                                          size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if ((theTag == nullptr && theTagLen != 0) || (theTag != nullptr && theTagLen == 0))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theTag and theTagLen are inconsistent");
      return OCCTL_INVALID_ARGUMENT;
    }

    const TCollection_AsciiString aFilterStorage =
      theTag != nullptr ? TCollection_AsciiString(theTag, static_cast<int>(theTagLen))
                        : TCollection_AsciiString();
    const TCollection_AsciiString* const aFilter = theTag != nullptr ? &aFilterStorage : nullptr;
    NCollection_LinearVector<BRepGraph_NodeId> aNodes;
    const occ::handle<OcctL_Topo_TagLayer>     aLayer = findTagLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->CollectActive(theGraph->graph, aFilter, aNodes);
    }
    return fillNodeBuffer(aNodes, theOutNodes, theCap, theOutCount, "occtl_graph_tag_nodes");
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_joint_info_init(occtl_joint_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_JOINT_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_joint_create(occtl_graph_t* const            theGraph,
                                                       const occtl_joint_info_t* const theInfo,
                                                       occtl_joint_id_t* const         theOutJoint)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutJoint == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph, theInfo, or theOutJoint is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    JointRecord aRecord;
    if (const occtl_status_t aStatus = jointRecordFromInfo(theGraph->graph, *theInfo, aRecord))
    {
      return aStatus;
    }

    occ::handle<OcctL_Topo_JointLayer> aLayer = findJointLayer(theGraph->graph);
    if (aLayer.IsNull())
    {
      OcctL::Topo::EnsureBuiltinLayers(theGraph->graph);
      aLayer = findJointLayer(theGraph->graph);
    }

    theOutJoint->bits = aLayer->CreateJoint(aRecord);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_joint_get(const occtl_graph_t* const theGraph,
                                                    const occtl_joint_id_t     theJoint,
                                                    occtl_joint_info_t* const  theOutInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutInfo is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theJoint.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theJoint is invalid");
      return OCCTL_INVALID_ARGUMENT;
    }

    const occ::handle<OcctL_Topo_JointLayer> aLayer = findJointLayer(theGraph->graph);
    const JointRecord* aRecord = !aLayer.IsNull() ? aLayer->FindJoint(theJoint.bits) : nullptr;
    if (aRecord == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "joint is not present");
      return OCCTL_NOT_FOUND;
    }

    fillJointInfo(*aRecord, *theOutInfo);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_joint_remove(occtl_graph_t* const   theGraph,
                                                       const occtl_joint_id_t theJoint)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theJoint.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theJoint is invalid");
      return OCCTL_INVALID_ARGUMENT;
    }

    const occ::handle<OcctL_Topo_JointLayer> aLayer = findJointLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->RemoveJoint(theJoint.bits);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_joint_list(const occtl_graph_t* const theGraph,
                                                     const occtl_node_id_t      theNode,
                                                     occtl_joint_id_t* const    theOutJoints,
                                                     const size_t               theCap,
                                                     size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theGraph or theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aFilter;
    if (theNode.bits != 0)
    {
      aFilter = OcctL::Topo::UnpackNodeId(theNode);
      if (!isActiveNode(theGraph->graph, aFilter))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theNode is invalid or removed");
        return OCCTL_NOT_FOUND;
      }
    }

    NCollection_LinearVector<occtl_joint_id_t> aJoints;
    const occ::handle<OcctL_Topo_JointLayer>   aLayer = findJointLayer(theGraph->graph);
    if (!aLayer.IsNull())
    {
      aLayer->Collect(aFilter, aJoints);
    }

    *theOutCount = aJoints.Size();
    if (theOutJoints == nullptr)
    {
      return OCCTL_OK;
    }

    if (theCap < aJoints.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theCap is too small for joint list");
      return OCCTL_BUFFER_TOO_SMALL;
    }

    for (size_t aJointIndex = 0; aJointIndex < aJoints.Size(); ++aJointIndex)
    {
      theOutJoints[aJointIndex] = aJoints.Value(aJointIndex);
    }
    return OCCTL_OK;
  });
}

} // extern "C"
