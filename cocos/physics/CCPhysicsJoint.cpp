/****************************************************************************
 Copyright (c) 2013 Chukong Technologies Inc.
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "physics/CCPhysicsJoint.h"
#if CC_USE_PHYSICS
#include "chipmunk.h"

#include "CCPhysicsBody.h"
#include "CCPhysicsWorld.h"
#include "CCPhysicsHelper.h"
#include "2d/CCNode.h"

NS_CC_BEGIN

PhysicsJoint::PhysicsJoint()
: _bodyA(nullptr)
, _bodyB(nullptr)
, _world(nullptr)
, _enable(false)
, _collisionEnable(true)
, _destoryMark(false)
, _tag(0)
{
    
}

PhysicsJoint::~PhysicsJoint()
{
    // reset the shapes collision group
    setCollisionEnable(true);
    
    for (auto constraint : _cpConstraints)
    {
        cpConstraintFree(constraint);
    }
}

bool PhysicsJoint::init(cocos2d::PhysicsBody *a, cocos2d::PhysicsBody *b)
{
    do
    {
        CCASSERT(a != nullptr && b != nullptr, "the body passed in is nil");
        CCASSERT(a != b, "the two bodies are equal");
        
        _bodyA = a;
        _bodyA->_joints.push_back(this);
        _bodyB = b;
        _bodyB->_joints.push_back(this);
        
        return true;
    } while (false);
    
    return false;
}

void PhysicsJoint::setEnable(bool enable)
{
    if (_enable != enable)
    {
        _enable = enable;
        
        if (_world != nullptr)
        {
            if (enable)
            {
                _world->addJointOrDelay(this);
            }else
            {
                _world->removeJointOrDelay(this);
            }
        }
    }
}

Node* PhysicsJoint::getBodyNode(PhysicsBody* body) const
{
    return body->_node;
}

void PhysicsJoint::setCollisionEnable(bool enable)
{
    if (_collisionEnable != enable)
    {
        _collisionEnable = enable;
    }
}

void PhysicsJoint::removeFormWorld()
{
    if (_world)
    {
        _world->removeJoint(this, false);
    }
}

void PhysicsJoint::destroy(PhysicsJoint* joint)
{
    if (joint!= nullptr)
    {
        // remove the joint and delete it.
        if (joint->_world != nullptr)
        {
            joint->_world->removeJoint(joint, true);
        }
        else
        {
            if (joint->_bodyA != nullptr)
            {
                joint->_bodyA->removeJoint(joint);
            }
            
            if (joint->_bodyB != nullptr)
            {
                joint->_bodyB->removeJoint(joint);
            }
            
            delete joint;
        }
    }
}

void PhysicsJoint::setMaxForce(float force)
{
    for (auto joint : _cpConstraints)
    {
        joint->maxForce = PhysicsHelper::float2cpfloat(force);
    }
}

float PhysicsJoint::getMaxForce() const
{
    return PhysicsHelper::cpfloat2float(_cpConstraints.front()->maxForce);
}

PhysicsJointFixed* PhysicsJointFixed::construct(PhysicsBody* a, PhysicsBody* b, const Vec2& anchr)
{
    PhysicsJointFixed* joint = new (std::nothrow) PhysicsJointFixed();
    
    if (joint && joint->init(a, b, anchr))
    {
        return joint;
    }
    
    CC_SAFE_DELETE(joint);
    return nullptr;
}

bool PhysicsJointFixed::init(PhysicsBody* a, PhysicsBody* b, const Vec2& anchr)
{
    do
    {
        CC_BREAK_IF(!PhysicsJoint::init(a, b));
        
        getBodyNode(a)->setPosition(anchr);
        getBodyNode(b)->setPosition(anchr);
        
        // add a pivot joint to fixed two body together
        auto constraint = cpPivotJointNew(a->getCPBody(), b->getCPBody(),
                                        PhysicsHelper::point2cpv(anchr));
        CC_BREAK_IF(constraint == nullptr);
        _cpConstraints.push_back(constraint);
        
        // add a gear joint to make two body have the same rotation.
        constraint = cpGearJointNew(a->getCPBody(), b->getCPBody(), 0, 1);
        CC_BREAK_IF(constraint == nullptr);
        _cpConstraints.push_back(constraint);
        
        setCollisionEnable(false);
        
        return true;
    } while (false);
    
    return false;
}

PhysicsJointPin* PhysicsJointPin::construct(PhysicsBody* a, PhysicsBody* b, const Vec2& anchr)
{
    PhysicsJointPin* joint = new (std::nothrow) PhysicsJointPin();
    
    if (joint && joint->init(a, b, anchr))
    {
        return joint;
    }
    
    CC_SAFE_DELETE(joint);
    return nullptr;
}

bool PhysicsJointPin::init(PhysicsBody *a, PhysicsBody *b, const Vec2& anchr)
{
    do
    {
        CC_BREAK_IF(!PhysicsJoint::init(a, b));
        auto constraint = cpPivotJointNew(a->getCPBody(), b->getCPBody(),
                                       PhysicsHelper::point2cpv(anchr));
        
        CC_BREAK_IF(constraint == nullptr);
        
        _cpConstraints.push_back(constraint);
        
        return true;
    } while (false);
    
    return false;
}

PhysicsJointLimit* PhysicsJointLimit::construct(PhysicsBody* a, PhysicsBody* b, const Vec2& anchr1, const Vec2& anchr2, float min, float max)
{
    PhysicsJointLimit* joint = new (std::nothrow) PhysicsJointLimit();
    
    if (joint && joint->init(a, b, anchr1, anchr2, min, max))
    {
        return joint;
    }
    
    CC_SAFE_DELETE(joint);
    return nullptr;
}

PhysicsJointLimit* PhysicsJointLimit::construct(PhysicsBody* a, PhysicsBody* b, const Vec2& anchr1, const Vec2& anchr2)
{
    return construct(a, b, anchr1, anchr2, 0, b->local2World(anchr1).getDistance(a->local2World(anchr2)));
}

bool PhysicsJointLimit::init(PhysicsBody* a, PhysicsBody* b, const Vec2& anchr1, const Vec2& anchr2, float min, float max)
{
    do
    {
        CC_BREAK_IF(!PhysicsJoint::init(a, b));
        
        auto constraint = cpSlideJointNew(a->getCPBody(), b->getCPBody(),
                                       PhysicsHelper::point2cpv(anchr1),
                                       PhysicsHelper::point2cpv(anchr2),
                                       PhysicsHelper::float2cpfloat(min),
                                       PhysicsHelper::float2cpfloat(max));
        
        CC_BREAK_IF(constraint == nullptr);
        
        _cpConstraints.push_back(constraint);
        
        return true;
    } while (false);
    
    return false;
}

float PhysicsJointLimit::getMin() const
{
    return PhysicsHelper::cpfloat2float(cpSlideJointGetMin(_cpConstraints.front()));
}

void PhysicsJointLimit::setMin(float min)
{
    cpSlideJointSetMin(_cpConstraints.front(), PhysicsHelper::float2cpfloat(min));
}

float PhysicsJointLimit::getMax() const
{
    return PhysicsHelper::cpfloat2float(cpSlideJointGetMax(_cpConstraints.front()));
}

void PhysicsJointLimit::setMax(float max)
{
    cpSlideJointSetMax(_cpConstraints.front(), PhysicsHelper::float2cpfloat(max));
}

Vec2 PhysicsJointLimit::getAnchr1() const
{
    return PhysicsHelper::cpv2point(cpSlideJointGetAnchr1(_cpConstraints.front()));
}

void PhysicsJointLimit::setAnchr1(const Vec2& anchr)
{
    cpSlideJointSetAnchr1(_cpConstraints.front(), PhysicsHelper::point2cpv(anchr));
}

Vec2 PhysicsJointLimit::getAnchr2() const
{
    return PhysicsHelper::cpv2point(cpSlideJointGetAnchr2(_cpConstraints.front()));
}

void PhysicsJointLimit::setAnchr2(const Vec2& anchr)
{
    cpSlideJointSetAnchr1(_cpConstraints.front(), PhysicsHelper::point2cpv(anchr));
}

PhysicsJointDistance* PhysicsJointDistance::construct(PhysicsBody* a, PhysicsBody* b, const Vec2& anchr1, const Vec2& anchr2)
{
    PhysicsJointDistance* joint = new (std::nothrow) PhysicsJointDistance();
    
    if (joint && joint->init(a, b, anchr1, anchr2))
    {
        return joint;
    }
    
    CC_SAFE_DELETE(joint);
    return nullptr;
}

bool PhysicsJointDistance::init(PhysicsBody* a, PhysicsBody* b, const Vec2& anchr1, const Vec2& anchr2)
{
    do
    {
        CC_BREAK_IF(!PhysicsJoint::init(a, b));
        
        auto constraint = cpPinJointNew(a->getCPBody(),
                                            b->getCPBody(),
                                            PhysicsHelper::point2cpv(anchr1),
                                            PhysicsHelper::point2cpv(anchr2));
        
        CC_BREAK_IF(constraint == nullptr);
        
        _cpConstraints.push_back(constraint);
        
        return true;
    } while (false);
    
    return false;
}

float PhysicsJointDistance::getDistance() const
{
    return PhysicsHelper::cpfloat2float(cpPinJointGetDist(_cpConstraints.front()));
}

void PhysicsJointDistance::setDistance(float distance)
{
    cpPinJointSetDist(_cpConstraints.front(), PhysicsHelper::float2cpfloat(distance));
}

PhysicsJointSpring* PhysicsJointSpring::construct(PhysicsBody* a, PhysicsBody* b, const Vec2& anchr1, const Vec2& anchr2, float stiffness, float damping)
{
    PhysicsJointSpring* joint = new (std::nothrow) PhysicsJointSpring();
    
    if (joint && joint->init(a, b, anchr1, anchr2, stiffness, damping))
    {
        return joint;
    }
    
    CC_SAFE_DELETE(joint);
    return nullptr;
}

bool PhysicsJointSpring::init(PhysicsBody* a, PhysicsBody* b, const Vec2& anchr1, const Vec2& anchr2, float stiffness, float damping)
{
    do {
        CC_BREAK_IF(!PhysicsJoint::init(a, b));
        
        auto constraint = cpDampedSpringNew(a->getCPBody(),
                                                b->getCPBody(),
                                                PhysicsHelper::point2cpv(anchr1),
                                                PhysicsHelper::point2cpv(anchr2),
                                                PhysicsHelper::float2cpfloat(_bodyB->local2World(anchr1).getDistance(_bodyA->local2World(anchr2))),
                                                PhysicsHelper::float2cpfloat(stiffness),
                                                PhysicsHelper::float2cpfloat(damping));
        
        CC_BREAK_IF(constraint == nullptr);
        
        _cpConstraints.push_back(constraint);
        
        return true;
    } while (false);
    
    return false;
}

Vec2 PhysicsJointSpring::getAnchr1() const
{
    return PhysicsHelper::cpv2point(cpDampedSpringGetAnchr1(_cpConstraints.front()));
}

void PhysicsJointSpring::setAnchr1(const Vec2& anchr)
{
    cpDampedSpringSetAnchr1(_cpConstraints.front(), PhysicsHelper::point2cpv(anchr));
}

Vec2 PhysicsJointSpring::getAnchr2() const
{
    return PhysicsHelper::cpv2point(cpDampedSpringGetAnchr2(_cpConstraints.front()));
}

void PhysicsJointSpring::setAnchr2(const Vec2& anchr)
{
    cpDampedSpringSetAnchr1(_cpConstraints.front(), PhysicsHelper::point2cpv(anchr));
}

float PhysicsJointSpring::getRestLength() const
{
    return PhysicsHelper::cpfloat2float(cpDampedSpringGetRestLength(_cpConstraints.front()));
}

void PhysicsJointSpring::setRestLength(float restLength)
{
    cpDampedSpringSetRestLength(_cpConstraints.front(), PhysicsHelper::float2cpfloat(restLength));
}

float PhysicsJointSpring::getStiffness() const
{
    return PhysicsHelper::cpfloat2float(cpDampedSpringGetStiffness(_cpConstraints.front()));
}

void PhysicsJointSpring::setStiffness(float stiffness)
{
    cpDampedSpringSetStiffness(_cpConstraints.front(), PhysicsHelper::float2cpfloat(stiffness));
}

float PhysicsJointSpring::getDamping() const
{
    return PhysicsHelper::cpfloat2float(cpDampedSpringGetDamping(_cpConstraints.front()));
}

void PhysicsJointSpring::setDamping(float damping)
{
    cpDampedSpringSetDamping(_cpConstraints.front(), PhysicsHelper::float2cpfloat(damping));
}

PhysicsJointGroove* PhysicsJointGroove::construct(PhysicsBody* a, PhysicsBody* b, const Vec2& grooveA, const Vec2& grooveB, const Vec2& anchr2)
{
    PhysicsJointGroove* joint = new (std::nothrow) PhysicsJointGroove();
    
    if (joint && joint->init(a, b, grooveA, grooveB, anchr2))
    {
        return joint;
    }
    
    CC_SAFE_DELETE(joint);
    return nullptr;
}

bool PhysicsJointGroove::init(PhysicsBody* a, PhysicsBody* b, const Vec2& grooveA, const Vec2& grooveB, const Vec2& anchr2)
{
    do {
        CC_BREAK_IF(!PhysicsJoint::init(a, b));
        
        auto constraint = cpGrooveJointNew(a->getCPBody(),
                                               b->getCPBody(),
                                               PhysicsHelper::point2cpv(grooveA),
                                               PhysicsHelper::point2cpv(grooveB),
                                               PhysicsHelper::point2cpv(anchr2));
        
        CC_BREAK_IF(constraint == nullptr);
        
        _cpConstraints.push_back(constraint);
        
        return true;
    } while (false);
    
    return false;
}

Vec2 PhysicsJointGroove::getGrooveA() const
{
    return PhysicsHelper::cpv2point(cpGrooveJointGetGrooveA(_cpConstraints.front()));
}

void PhysicsJointGroove::setGrooveA(const Vec2& grooveA)
{
    cpGrooveJointSetGrooveA(_cpConstraints.front(), PhysicsHelper::point2cpv(grooveA));
}

Vec2 PhysicsJointGroove::getGrooveB() const
{
    return PhysicsHelper::cpv2point(cpGrooveJointGetGrooveB(_cpConstraints.front()));
}

void PhysicsJointGroove::setGrooveB(const Vec2& grooveB)
{
    cpGrooveJointSetGrooveB(_cpConstraints.front(), PhysicsHelper::point2cpv(grooveB));
}

Vec2 PhysicsJointGroove::getAnchr2() const
{
    return PhysicsHelper::cpv2point(cpGrooveJointGetAnchr2(_cpConstraints.front()));
}

void PhysicsJointGroove::setAnchr2(const Vec2& anchr2)
{
    cpGrooveJointSetAnchr2(_cpConstraints.front(), PhysicsHelper::point2cpv(anchr2));
}

PhysicsJointRotarySpring* PhysicsJointRotarySpring::construct(PhysicsBody* a, PhysicsBody* b, float stiffness, float damping)
{
    PhysicsJointRotarySpring* joint = new (std::nothrow) PhysicsJointRotarySpring();
    
    if (joint && joint->init(a, b, stiffness, damping))
    {
        return joint;
    }
    
    CC_SAFE_DELETE(joint);
    return nullptr;
}

bool PhysicsJointRotarySpring::init(PhysicsBody* a, PhysicsBody* b, float stiffness, float damping)
{
    do {
        CC_BREAK_IF(!PhysicsJoint::init(a, b));
        
        auto constraint = cpDampedRotarySpringNew(a->getCPBody(),
                                                      b->getCPBody(),
                                                      PhysicsHelper::float2cpfloat(_bodyB->getRotation() - _bodyA->getRotation()),
                                                      PhysicsHelper::float2cpfloat(stiffness),
                                                      PhysicsHelper::float2cpfloat(damping));
        
        CC_BREAK_IF(constraint == nullptr);
        
        _cpConstraints.push_back(constraint);
        
        return true;
    } while (false);
    
    return false;
}

float PhysicsJointRotarySpring::getRestAngle() const
{
    return PhysicsHelper::cpfloat2float(cpDampedRotarySpringGetRestAngle(_cpConstraints.front()));
}

void PhysicsJointRotarySpring::setRestAngle(float restAngle)
{
    cpDampedRotarySpringSetRestAngle(_cpConstraints.front(), PhysicsHelper::float2cpfloat(restAngle));
}

float PhysicsJointRotarySpring::getStiffness() const
{
    return PhysicsHelper::cpfloat2float(cpDampedRotarySpringGetStiffness(_cpConstraints.front()));
}

void PhysicsJointRotarySpring::setStiffness(float stiffness)
{
    cpDampedRotarySpringSetStiffness(_cpConstraints.front(), PhysicsHelper::float2cpfloat(stiffness));
}

float PhysicsJointRotarySpring::getDamping() const
{
    return PhysicsHelper::cpfloat2float(cpDampedRotarySpringGetDamping(_cpConstraints.front()));
}

void PhysicsJointRotarySpring::setDamping(float damping)
{
    cpDampedRotarySpringSetDamping(_cpConstraints.front(), PhysicsHelper::float2cpfloat(damping));
}

PhysicsJointRotaryLimit* PhysicsJointRotaryLimit::construct(PhysicsBody* a, PhysicsBody* b, float min, float max)
{
    PhysicsJointRotaryLimit* joint = new (std::nothrow) PhysicsJointRotaryLimit();
    
    if (joint && joint->init(a, b, min, max))
    {
        return joint;
    }
    
    CC_SAFE_DELETE(joint);
    return nullptr;
}

PhysicsJointRotaryLimit* PhysicsJointRotaryLimit::construct(PhysicsBody* a, PhysicsBody* b)
{
    return construct(a, b, 0.0f, 0.0f);
}

bool PhysicsJointRotaryLimit::init(PhysicsBody* a, PhysicsBody* b, float min, float max)
{
    do
    {
        CC_BREAK_IF(!PhysicsJoint::init(a, b));
        
        auto constraint = cpRotaryLimitJointNew(a->getCPBody(),
                                                    b->getCPBody(),
                                                    PhysicsHelper::float2cpfloat(min),
                                                    PhysicsHelper::float2cpfloat(max));
        
        CC_BREAK_IF(constraint == nullptr);
        
        _cpConstraints.push_back(constraint);
        
        return true;
    } while (false);
    
    return false;
}

float PhysicsJointRotaryLimit::getMin() const
{
    return PhysicsHelper::cpfloat2float(cpRotaryLimitJointGetMin(_cpConstraints.front()));
}

void PhysicsJointRotaryLimit::setMin(float min)
{
    cpRotaryLimitJointSetMin(_cpConstraints.front(), PhysicsHelper::float2cpfloat(min));
}

float PhysicsJointRotaryLimit::getMax() const
{
    return PhysicsHelper::cpfloat2float(cpRotaryLimitJointGetMax(_cpConstraints.front()));
}

void PhysicsJointRotaryLimit::setMax(float max)
{
    cpRotaryLimitJointSetMax(_cpConstraints.front(), PhysicsHelper::float2cpfloat(max));
}

PhysicsJointRatchet* PhysicsJointRatchet::construct(PhysicsBody* a, PhysicsBody* b, float phase, float ratchet)
{
    PhysicsJointRatchet* joint = new (std::nothrow) PhysicsJointRatchet();
    
    if (joint && joint->init(a, b, phase, ratchet))
    {
        return joint;
    }
    
    CC_SAFE_DELETE(joint);
    return nullptr;
}

bool PhysicsJointRatchet::init(PhysicsBody* a, PhysicsBody* b, float phase, float ratchet)
{
    do
    {
        CC_BREAK_IF(!PhysicsJoint::init(a, b));
        
        auto constraint = cpRatchetJointNew(a->getCPBody(),
                                                b->getCPBody(),
                                                PhysicsHelper::float2cpfloat(phase),
                                                PhysicsHelper::cpfloat2float(ratchet));
        
        CC_BREAK_IF(constraint == nullptr);
        
        _cpConstraints.push_back(constraint);
        
        return true;
    } while (false);
    
    return false;
}

float PhysicsJointRatchet::getAngle() const
{
    return PhysicsHelper::cpfloat2float(cpRatchetJointGetAngle(_cpConstraints.front()));
}

void PhysicsJointRatchet::setAngle(float angle)
{
    cpRatchetJointSetAngle(_cpConstraints.front(), PhysicsHelper::float2cpfloat(angle));
}

float PhysicsJointRatchet::getPhase() const
{
    return PhysicsHelper::cpfloat2float(cpRatchetJointGetPhase(_cpConstraints.front()));
}

void PhysicsJointRatchet::setPhase(float phase)
{
    cpRatchetJointSetPhase(_cpConstraints.front(), PhysicsHelper::float2cpfloat(phase));
}

float PhysicsJointRatchet::getRatchet() const
{
    return PhysicsHelper::cpfloat2float(cpRatchetJointGetRatchet(_cpConstraints.front()));
}

void PhysicsJointRatchet::setRatchet(float ratchet)
{
    cpRatchetJointSetRatchet(_cpConstraints.front(), PhysicsHelper::float2cpfloat(ratchet));
}

PhysicsJointGear* PhysicsJointGear::construct(PhysicsBody* a, PhysicsBody* b, float phase, float ratchet)
{
    PhysicsJointGear* joint = new (std::nothrow) PhysicsJointGear();
    
    if (joint && joint->init(a, b, phase, ratchet))
    {
        return joint;
    }
    
    CC_SAFE_DELETE(joint);
    return nullptr;
}

bool PhysicsJointGear::init(PhysicsBody* a, PhysicsBody* b, float phase, float ratio)
{
    do
    {
        CC_BREAK_IF(!PhysicsJoint::init(a, b));
        
        auto constraint = cpGearJointNew(a->getCPBody(),
                                             b->getCPBody(),
                                             PhysicsHelper::float2cpfloat(phase),
                                             PhysicsHelper::float2cpfloat(ratio));
        
        CC_BREAK_IF(constraint == nullptr);
        
        _cpConstraints.push_back(constraint);
        
        return true;
    } while (false);
    
    return false;
}

float PhysicsJointGear::getPhase() const
{
    return PhysicsHelper::cpfloat2float(cpGearJointGetPhase(_cpConstraints.front()));
}

void PhysicsJointGear::setPhase(float phase)
{
    cpGearJointSetPhase(_cpConstraints.front(), PhysicsHelper::float2cpfloat(phase));
}

float PhysicsJointGear::getRatio() const
{
    return PhysicsHelper::cpfloat2float(cpGearJointGetRatio(_cpConstraints.front()));
}

void PhysicsJointGear::setRatio(float ratio)
{
    cpGearJointSetRatio(_cpConstraints.front(), PhysicsHelper::float2cpfloat(ratio));
}

PhysicsJointMotor* PhysicsJointMotor::construct(PhysicsBody* a, PhysicsBody* b, float rate)
{
    PhysicsJointMotor* joint = new (std::nothrow) PhysicsJointMotor();
    
    if (joint && joint->init(a, b, rate))
    {
        return joint;
    }
    
    CC_SAFE_DELETE(joint);
    return nullptr;
}

bool PhysicsJointMotor::init(PhysicsBody* a, PhysicsBody* b, float rate)
{
    do
    {
        CC_BREAK_IF(!PhysicsJoint::init(a, b));
        
        auto constraint = cpSimpleMotorNew(a->getCPBody(),
                                             b->getCPBody(),
                                             PhysicsHelper::float2cpfloat(rate));
        
        CC_BREAK_IF(constraint == nullptr);
        
        _cpConstraints.push_back(constraint);
        
        return true;
    } while (false);
    
    return false;
}

float PhysicsJointMotor::getRate() const
{
    return PhysicsHelper::cpfloat2float(cpSimpleMotorGetRate(_cpConstraints.front()));
}

void PhysicsJointMotor::setRate(float rate)
{
    cpSimpleMotorSetRate(_cpConstraints.front(), PhysicsHelper::float2cpfloat(rate));
}

NS_CC_END
#endif // CC_USE_PHYSICS
