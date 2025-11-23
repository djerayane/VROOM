#include "vroom/core/Scene.hpp"
#include "vroom/logging/LogMacros.hpp"

namespace vroom {

Scene::Scene() {
    LOG_ENGINE_CLASS_DEBUG("Scene created");
}

Scene::~Scene() {
    LOG_ENGINE_CLASS_DEBUG("Scene destroyed");
    clear();
}

std::shared_ptr<Entity> Scene::createEntity(const glm::vec3& position,
                                            const glm::vec3& rotation,
                                            const glm::vec3& scale) {
    auto entity = std::make_shared<Entity>(generateEntityId(), shared_from_this(), position, rotation, scale);
    m_entities.push_back(entity);
    LOG_ENGINE_CLASS_DEBUG("Created Entity ID: " + std::to_string(entity->getId()));
    return entity;
}

void Scene::destroyEntity(Entity& entity) {
    // First, handle children recursively
    // We make a copy of the children list because it will be modified as we destroy them
    std::vector<Entity*> children = entity.getChildren();
    for (auto* child : children) {
        if (child) {
            destroyEntity(*child);
        }
    }

    // Remove from parent if exists
    if (auto* parent = entity.getParent()) {
        parent->removeChild(&entity);
    }

    // Remove from scene storage
    auto it = std::remove_if(m_entities.begin(), m_entities.end(), 
        [&entity](const std::shared_ptr<Entity>& e) {
            return e.get() == &entity;
        });
    
    if (it != m_entities.end()) {
        m_entities.erase(it, m_entities.end());
    }
}

void Scene::update(float deltaTime) {
    // Iterate using index to allow safe addition of entities during update (though removal might be tricky)
    // Ideally we'd have a "pending addition" and "pending removal" list, but for now direct iteration
    // We copy the vector to avoid iterator invalidation if entities are added/removed
    auto entitiesSnapshot = m_entities;
    
    for (const auto& entity : entitiesSnapshot) {
        // Only update root entities to avoid double updates, as Entity::update handles children recursively
        if (entity && entity->isActive() && entity->getParent() == nullptr) {
             entity->update(deltaTime);
        }
    }
}

void Scene::clear() {
    if (!m_entities.empty()) {
        LOG_ENGINE_CLASS_INFO("Clearing scene, destroying " + std::to_string(m_entities.size()) + " entities");
        m_entities.clear();
    }
    m_nextEntityId = 1;
}

std::vector<Entity*> Scene::getRootEntities() const {
    std::vector<Entity*> roots;
    for (const auto& entity : m_entities) {
        if (entity->getParent() == nullptr) {
            roots.push_back(entity.get());
        }
    }
    return roots;
}

EntityId Scene::generateEntityId() {
    return m_nextEntityId++;
}

} // namespace vroom
