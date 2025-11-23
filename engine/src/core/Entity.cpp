#include "vroom/core/Entity.hpp"
#include "vroom/core/Scene.hpp"
#include "vroom/components/Transform.hpp"
#include "vroom/logging/LogMacros.hpp"
#include <algorithm>
#include <string>

namespace vroom {

Entity::Entity(EntityId id, std::shared_ptr<Scene> scene,
               const glm::vec3& position,
               const glm::vec3& rotation,
               const glm::vec3& scale)
    : m_id(id), m_scene(std::move(scene)) {
    addComponent<Transform>(position, rotation, scale);
}

Entity::~Entity() {
    // LOG_ENGINE_CLASS_DEBUG("Destroying Entity ID: " + std::to_string(m_id)); // Too spammy for regular runs
    
    // Detach children first
    // We create a copy because setParent(nullptr) modifies the m_children vector
    auto childrenCopy = m_children;
    for (auto* child : childrenCopy) {
        if (child) {
            child->setParent(nullptr);
        }
    }

    // Detach from parent
    if (m_parent) {
        auto& parentChildren = m_parent->m_children;
        auto it = std::find(parentChildren.begin(), parentChildren.end(), this);
        if (it != parentChildren.end()) {
            parentChildren.erase(it);
        }
    }

    for (auto& component : m_components) {
        component->onDestroy();
    }
    m_components.clear();
}

bool Entity::isActive() const {
    return m_active && (!m_parent || m_parent->isActive());
}

void Entity::update(float deltaTime) {
    if (!isActive()) {
        return;
    }

    for (auto& component : m_components) {
        if (component->isEnabled()) {
            if (!component->hasStarted()) {
                component->start();
                component->markStarted();
            }
            
            // Check enabled again in case start() disabled the component
            if (component->isEnabled()) {
                component->update(deltaTime);
            }
        }
    }

    for (auto* child : m_children) {
        child->update(deltaTime);
    }
}

void Entity::setActive(bool active) {
    if (m_active == active) {
        return;
    }

    bool wasActive = isActive();
    m_active = active;
    bool isNowActive = isActive();

    if (wasActive != isNowActive) {
        handleActiveStateChange(wasActive, isNowActive);
    }
}

void Entity::handleActiveStateChange(bool wasActive, bool isNowActive) {
    // Notify components
    for (auto& component : m_components) {
        if (component->isEnabled()) {
            if (isNowActive) {
                component->onEnable();
            } else {
                component->onDisable();
            }
        }
    }

    // Notify children
    for (auto* child : m_children) {
        // Child's local active state hasn't changed, but effective state might have
        // If child.m_active is true, then its effective state follows parent
        if (child->m_active) {
            child->handleActiveStateChange(wasActive, isNowActive);
        }
    }
}

void Entity::setParent(Entity* parent) {
    if (m_parent == parent) {
        return;
    }

    // Prevent cycles: check if new parent is a descendant of this entity
    Entity* p = parent;
    while (p) {
        if (p == this) {
            LOG_ENGINE_CLASS_WARNING("Attempted to set parent that would create a cycle (Entity ID: " + std::to_string(m_id) + ")");
            return;
        }
        p = p->getParent();
    }

    bool wasActive = isActive();

    if (m_parent) {
        auto& parentChildren = m_parent->m_children;
        auto it = std::find(parentChildren.begin(), parentChildren.end(), this);
        if (it != parentChildren.end()) {
            parentChildren.erase(it);
        }
    }

    m_parent = parent;

    if (m_parent) {
        m_parent->m_children.push_back(this);
    }

    bool isNowActive = isActive();
    if (wasActive != isNowActive) {
        handleActiveStateChange(wasActive, isNowActive);
    }
}

void Entity::addChild(Entity* child) {
    if (child) {
        child->setParent(this);
    }
}

void Entity::removeChild(Entity* child) {
    if (child && child->getParent() == this) {
        child->setParent(nullptr);
    }
}

SceneManager* Entity::getSceneManager() const {
    if (auto scene = m_scene.lock()) {
        return scene->getSceneManager();
    }
    return nullptr;
}

} // namespace vroom
