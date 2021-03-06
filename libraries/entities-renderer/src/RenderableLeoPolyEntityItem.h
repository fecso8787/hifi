//
//  RenderableLeoPolyEntityItem.h
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 2016-11-5.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableLeoPolyEntityItem_h
#define hifi_RenderableLeoPolyEntityItem_h

#include <QSemaphore>
#include <atomic>

#include <TextureCache.h>

#include "LeoPolyEntityItem.h"
#include "RenderableEntityItem.h"
#include "gpu/Context.h"
#include <AssetClient.h>
#include <AssetRequest.h>
#include <AssetUpload.h>

class LeoPolyPayload {
public:
    LeoPolyPayload(EntityItemPointer owner) : _owner(owner), _bounds(AABox()) { }
    typedef render::Payload<LeoPolyPayload> Payload;
    typedef Payload::DataPointer Pointer;

    EntityItemPointer _owner;
    AABox _bounds;
};

namespace render {
   template <> const ItemKey payloadGetKey(const LeoPolyPayload::Pointer& payload);
   template <> const Item::Bound payloadGetBound(const LeoPolyPayload::Pointer& payload);
   template <> void payloadRender(const LeoPolyPayload::Pointer& payload, RenderArgs* args);
}


class RenderableLeoPolyEntityItem : public LeoPolyEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableLeoPolyEntityItem(const EntityItemID& entityItemID);

    virtual ~RenderableLeoPolyEntityItem();

    virtual void somethingChangedNotification() override {
        // This gets called from EnityItem::readEntityDataFromBuffer every time a packet describing
        // this entity comes from the entity-server.  It gets called even if nothing has actually changed
        // (see the comment in EntityItem.cpp).  If that gets fixed, this could be used to know if we
        // need to redo the voxel data.
        // _needsModelReload = true;
    }
    void setLeoPolyModelVersion(QUuid value)override
    {
        if (_modelVersion != value && getEntityItemID() != getCurrentlyEditingEntityID())
        {
            _modelResource.reset();
            _mesh.reset();
        }
        _modelVersion = value;
    }
    virtual void render(RenderArgs* args) override;
    virtual void update(const quint64& now) override;
    
    virtual bool supportsDetailedRayIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                        bool& keepSearching, OctreeElementPointer& element, float& distance,
                        BoxFace& face, glm::vec3& surfaceNormal,
                        void** intersectedObject, bool precisionPicking) const override;

    virtual ShapeType getShapeType() const override;
    virtual bool shouldBePhysical() const override { return !isDead(); }
    virtual bool isReadyToComputeShape() override;
    virtual void computeShapeInfo(ShapeInfo& info) override;

    virtual bool addToScene(EntityItemPointer self,
                            std::shared_ptr<render::Scene> scene,
                            render::PendingChanges& pendingChanges) override;
    virtual void removeFromScene(EntityItemPointer self,
                                 std::shared_ptr<render::Scene> scene,
                                 render::PendingChanges& pendingChanges) override;

    virtual void updateRegistrationPoint(const glm::vec3& value) override;

    bool isTransparent() override { return false; }

    void getMesh();
    void setMesh(model::MeshPointer mesh);

    // helper function for determining which entity is currently under sculpt.
    static EntityItemID getCurrentlyEditingEntityID();

    void setUnderSculpting(bool value); // makes the current entity become the actively sculpted entity

    //Exports current model to an external storage via LeoEngine
    void doExportCurrentState()override;

    //Sends the actual geometry data to the LeoPolyEngine
    void sendToLeoEngine(ModelPointer model) override;

private:
    struct VertexStateChange
    {
        enum VertexStateChangeType{ Modified, Added, Deleted };
        unsigned int index;
        glm::vec3 newValue;
        VertexStateChangeType type;
    };
    struct VertexNormalTexCoord {
        glm::vec3 vertex;
        glm::vec3 normal;
        glm::vec2 texCoord;
        static gpu::Stream::FormatPointer getVertexFormat() {
            static const gpu::Element POSITION_ELEMENT{ gpu::VEC3, gpu::FLOAT, gpu::XYZ };
            static const gpu::Element NORMAL_ELEMENT{ gpu::VEC3, gpu::FLOAT, gpu::XYZ };
            static const gpu::Element TEXTURE_ELEMENT{ gpu::VEC2, gpu::FLOAT, gpu::UV };
            
            gpu::Stream::FormatPointer vertexFormat{ std::make_shared<gpu::Stream::Format>() };
            vertexFormat->setAttribute(gpu::Stream::POSITION, 0, POSITION_ELEMENT, offsetof(VertexNormalTexCoord, vertex));
            vertexFormat->setAttribute(gpu::Stream::NORMAL, 0, NORMAL_ELEMENT, offsetof(VertexNormalTexCoord, normal));
            vertexFormat->setAttribute(gpu::Stream::TEXCOORD, 0, TEXTURE_ELEMENT, offsetof(VertexNormalTexCoord, texCoord));
            
            return vertexFormat;
        }
    };
    std::vector<VertexStateChange> getDiffFromPreviousState(QVector<glm::vec3>)const;
    void updateGeometryFromLeoPlugin();
    void createShaderPipeline();
    void importToLeoPoly();
    void initializeModelResource();

    const int MATERIAL_GPU_SLOT = 3;
    render::ItemID _myItem{ render::Item::INVALID_ITEM_ID };
    static gpu::PipelinePointer _pipeline;

    model::MeshPointer _mesh;
    std::vector<model::Mesh::Part> _meshParts;
    std::vector<LeoPlugin::IncomingMaterial> _materials;
    GeometryResource::Pointer _modelResource;

    ShapeInfo _shapeInfo;
    static model::Box evalMeshBound(const model::MeshPointer mesh);
    static bool doUploadViaFTP(std::string fileName);
};

#endif // hifi_RenderableLeoPolyEntityItem_h
