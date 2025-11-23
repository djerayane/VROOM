#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include "vroom/core/Entity.hpp"

namespace vroom {

class Scene : public std::enable_shared_from_this<Scene> {
public:
    Scene();
    ~Scene();

    /// \brief Creates a new entity in the scene with optional transform values.
    /// \param position Initial position.
    /// \param rotation Initial rotation in degrees.
    /// \param scale Initial scale.
    /// \return Reference to the newly created entity.
    std::shared_ptr<Entity> createEntity(const glm::vec3& position = glm::vec3(0.0f),
                                         const glm::vec3& rotation = glm::vec3(0.0f),
                                         const glm::vec3& scale = glm::vec3(1.0f));

    /// \brief Sets the SceneManager for this scene.
    /// \param sceneManager Pointer to the SceneManager.
    void setSceneManager(SceneManager* sceneManager) { m_sceneManager = sceneManager; }

    /// \brief Gets the SceneManager associated with this scene.
    /// \return Pointer to the SceneManager.
    SceneManager* getSceneManager() const { return m_sceneManager; }

    /// \brief Destroys an entity and all its children.
    /// \param entity The entity to destroy.
    void destroyEntity(Entity& entity);

    /// \brief Updates all entities in the scene.
    /// \param deltaTime Time elapsed since the last frame.
    void update(float deltaTime);

    /// \brief Removes all entities from the scene.
    void clear();

    /// \brief Gets all root entities in the scene.
    /// \return Vector of pointers to root entities.
    std::vector<Entity*> getRootEntities() const;

private:
    EntityId m_nextEntityId = 1;
    std::vector<std::shared_ptr<Entity>> m_entities;
    SceneManager* m_sceneManager = nullptr;
    
    // Helper to generate unique IDs
    EntityId generateEntityId();
};

} // namespace vroom
