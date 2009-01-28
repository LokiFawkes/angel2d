%module(directors="1") angel
%{
#include "../../Physics/PhysicsActor.h"
%}

class PhysicsActor : public Actor
{
public:
	PhysicsActor();
	virtual ~PhysicsActor();
	
	enum eShapeType
	{
		SHAPETYPE_BOX,
		SHAPETYPE_CIRCLE,
	};
	
	void SetDensity(float density);
	void SetFriction(float friction);
	void SetRestitution(float restitution);
	void SetShapeType(eShapeType shapeType);
	void SetIsSensor(bool isSensor);
	void SetGroupIndex(int groupIndex);
	void SetCollisionFlags(int collisionFlags);
	void SetFixedRotation(bool fixedRotation);
	
	virtual void InitPhysics();
	virtual void CustomInitPhysics() {}
	
	void ApplyForce(const Vector2& force, const Vector2& point);
	void ApplyLocalForce(const Vector2& force, const Vector2& point);	// apply a local space force on the object
	void ApplyTorque(float torque);
	void ApplyImpulse(const Vector2& impulse, const Vector2& point);
	
	void SetSize(float x, float y = -1.f);
	void SetDrawSize(float x, float y = -1.f);
	void SetPosition(float x, float y);
	void SetPosition(Vector2 pos);
	void SetRotation(float rotation);
};

