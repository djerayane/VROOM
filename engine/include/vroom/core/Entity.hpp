#pragma once

#include <memory>
#include <vector>
#include <type_traits>

#include "vroom/core/Component.hpp"
#include <glm/glm.hpp>

namespace vroom {

class Scene;

using EntityId = uint64_t;
constexpr EntityId INVALID_ENTITY_ID = 0;

/// \brief Represents an entity in the ECS architecture.
///
/// Entities are container objects that hold components. They are identified by a unique ID
/// and belong to a Scene.
class Entity {
public:
    Entity() = default;
    
    /// \brief Constructs an entity with a specific ID and scene.
    /// \param id The unique identifier for the entity.
    /// \param scene The scene this entity belongs to.
    /// \param position Optional initial position.
    /// \param rotation Optional initial rotation in degrees.
    /// \param scale Optional initial scale.
    Entity(EntityId id, std::shared_ptr<Scene> scene, 
           const glm::vec3& position = glm::vec3(0.0f),
           const glm::vec3& rotation = glm::vec3(0.0f),
           const glm::vec3& scale = glm::vec3(1.0f));
    
    ~Entity();

    /// \brief Gets the unique identifier of the entity.
    /// \return The entity ID.
    EntityId getId() const { return m_id; }

    /// \brief Updates the entity and all its enabled components.
    /// \param deltaTime Time elapsed since the last frame.
    void update(float deltaTime);

    /// \brief Sets the active state of the entity.
    /// \param active The new active state.
    void setActive(bool active);

    /// \brief Checks if the entity is active.
    /// \return True if active, false otherwise.
    bool isActive() const;

    /// \brief Adds a component to the entity.
    /// \tparam T The type of component to add. Must inherit from Component.
    /// \tparam Args Variadic types for the component constructor arguments.
    /// \param args Arguments forwarded to the component's constructor.
    /// \return Reference to the newly created component.
    template <typename T, typename... Args>
    T& addComponent(Args&&... args) {
        static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
        
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        component->setEntity(this);
        
        component->awake();

        if (component->isEnabled() && isActive()) {
            component->onEnable();
        }
        
        T* componentPtr = component.get();
        m_components.push_back(std::move(component));
        return *componentPtr;
    }

    /// \brief Retrieves a component of a specific type.
    /// \tparam T The type of component to retrieve.
    /// \return Pointer to the component if found, nullptr otherwise.
    template <typename T>
    T* getComponent() {
        for (auto& component : m_components) {
            if (T* casted = dynamic_cast<T*>(component.get())) {
                return casted;
            }
        }
        return nullptr;
    }

    /// \brief Retrieves a constant component of a specific type.
    /// \tparam T The type of component to retrieve.
    /// \return Const pointer to the component if found, nullptr otherwise.
    template <typename T>
    const T* getComponent() const {
        for (const auto& component : m_components) {
            if (const T* casted = dynamic_cast<const T*>(component.get())) {
                return casted;
            }
        }
        return nullptr;
    }

    /// \brief Sets the parent of this entity.
    /// \param parent The new parent entity.
    void setParent(Entity* parent);

    /// \brief Gets the parent of this entity.
    /// \return Pointer to the parent entity.
    Entity* getParent() const { return m_parent; }

    /// \brief Adds a child entity.
    /// \param child The child entity to add.
    void addChild(Entity* child);

    /// \brief Removes a child entity.
    /// \param child The child entity to remove.
    void removeChild(Entity* child);

    /// \brief Gets the children of this entity.
    /// \return Reference to the vector of children.
    const std::vector<Entity*>& getChildren() const { return m_children; }

    /// \brief Gets the SceneManager that owns the scene this entity is in.
    /// \return Pointer to the SceneManager, or nullptr if not attached.
    class SceneManager* getSceneManager() const;

private:
    /// \brief Helper to handle active state changes recursively.
    void handleActiveStateChange(bool wasActive, bool isNowActive);

    EntityId m_id = INVALID_ENTITY_ID;
    std::weak_ptr<Scene> m_scene;
    std::vector<std::unique_ptr<Component>> m_components;
    bool m_active = true;
    Entity* m_parent = nullptr;
    std::vector<Entity*> m_children;
};

} // namespace vroom
