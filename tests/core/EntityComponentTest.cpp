#include <gtest/gtest.h>
#include "vroom/core/Entity.hpp"
#include "vroom/core/Component.hpp"

// Mock Scene since we don't have a definition yet
namespace vroom {
class Scene {};
}

class PositionComponent : public vroom::Component {
public:
    PositionComponent(float x, float y, float z) : x(x), y(y), z(z) {}
    float x, y, z;
};

class VelocityComponent : public vroom::Component {
public:
    VelocityComponent(float vx, float vy, float vz) : vx(vx), vy(vy), vz(vz) {}
    float vx, vy, vz;
};

TEST(EntityComponentTest, AddAndGetComponent) {
    vroom::Entity entity(1, nullptr);

    // Fix arguments to match Transform constructor being called by Entity constructor or avoid if manual
    // Entity constructor adds Transform automatically now.
    // But here we are testing Entity independently without Scene, using dummy args.
    
    auto& pos = entity.addComponent<PositionComponent>(10.0f, 20.0f, 30.0f);
    EXPECT_EQ(pos.x, 10.0f);
    EXPECT_EQ(pos.y, 20.0f);
    EXPECT_EQ(pos.z, 30.0f);
    
    // Check parenting
    EXPECT_EQ(pos.getEntity(), &entity);

    auto* retrievedPos = entity.getComponent<PositionComponent>();
    ASSERT_NE(retrievedPos, nullptr);
    EXPECT_EQ(retrievedPos->x, 10.0f);
    EXPECT_EQ(retrievedPos->y, 20.0f);
    EXPECT_EQ(retrievedPos->z, 30.0f);
    EXPECT_EQ(retrievedPos, &pos);
}

TEST(EntityComponentTest, GetMissingComponent) {
    vroom::Entity entity(1, nullptr);
    
    auto* pos = entity.getComponent<PositionComponent>();
    EXPECT_EQ(pos, nullptr);
}

TEST(EntityComponentTest, AddMultipleComponents) {
    vroom::Entity entity(1, nullptr);

    entity.addComponent<PositionComponent>(1.0f, 2.0f, 3.0f);
    entity.addComponent<VelocityComponent>(0.1f, 0.2f, 0.3f);

    auto* pos = entity.getComponent<PositionComponent>();
    auto* vel = entity.getComponent<VelocityComponent>();

    ASSERT_NE(pos, nullptr);
    ASSERT_NE(vel, nullptr);

    EXPECT_EQ(pos->x, 1.0f);
    EXPECT_EQ(vel->vx, 0.1f);
}

TEST(EntityComponentTest, ConstEntityGetComponent) {
    vroom::Entity entity(1, nullptr);
    entity.addComponent<PositionComponent>(5.0f, 5.0f, 5.0f);

    const vroom::Entity& constEntity = entity;
    const auto* pos = constEntity.getComponent<PositionComponent>();
    
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->x, 5.0f);
}

class BaseScript : public vroom::Component {
public:
    virtual std::string getName() const { return "Base"; }
};

class MyScript : public BaseScript {
public:
    std::string getName() const override { return "MyScript"; }
};

TEST(EntityComponentTest, ComponentInheritance) {
    vroom::Entity entity(1, nullptr);
    entity.addComponent<MyScript>();

    // Should be able to get as MyScript
    auto* myScript = entity.getComponent<MyScript>();
    ASSERT_NE(myScript, nullptr);
    EXPECT_EQ(myScript->getName(), "MyScript");

    // Should be able to get as BaseScript
    auto* baseScript = entity.getComponent<BaseScript>();
    ASSERT_NE(baseScript, nullptr);
    EXPECT_EQ(baseScript->getName(), "MyScript");
}

