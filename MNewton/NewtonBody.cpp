#include "NewtonBody.h"
#include "NewtonWorld.h"
#include "NewtonShape.h"
#include "NewtonMaterial.h"
#include "NewtonUtil.h"

#define GRAVITY	-1000.0f

namespace Myway {

	namespace Newton {

		tBody::tBody(tShape * shape, int type)
			: mShape(shape)
			, mType(type)
			, mNode(NULL)
			, mBody(NULL)
		{
		}

		tBody::~tBody()
		{
			NewtonDestroyBody(tWorld::Instance()->_getNewtonWorld(), mBody);
		}

		void tBody::SetBodyMatrix(const Mat4 & worldTm)
		{
			NewtonBodySetMatrix(mBody, &Util_Mat4_2_dMat(worldTm)[0][0]);
		}

		Vec4 tBody::GetMassMatrix() const
		{
			Vec4 massMat;

			NewtonBodyGetMassMatrix(mBody, &massMat.w, &massMat.x, &massMat.y, &massMat.z);

			return massMat;
		}

		void tBody::SetCenterOfMass(Vec3 & center)
		{
			NewtonBodySetCentreOfMass(mBody, &center.x);
		}

		Vec3 tBody::GetCenterOfMass() const
		{
			Vec3 center;

			NewtonBodyGetCentreOfMass(mBody, &center.x);

			return center;
		}

		void tBody::SetMaterialId(const tMaterialId * matId)
		{
			NewtonBodySetMaterialGroupID(mBody, matId->GetId());
		}

		void tBody::SetVelocity(const Vec3 & v)
		{
			NewtonBodySetVelocity(mBody, &v.x);
		}

		Vec3 tBody::GetVelocity() const
		{
			Vec3 v;

			NewtonBodyGetVelocity(mBody, &v.x);

			return v;
		}







		void DestroyBodyCallback (const NewtonBody* body)
		{
			tBody * me = (tBody *)NewtonBodyGetUserData(body);

			me->OnDestroy(NULL, NULL);
		}

		// Transform callback to set the matrix of a the visual entity
		void SetTransformCallback (const NewtonBody* body, const dFloat* matrix, int threadIndex)
		{
			Vec3 posit (matrix[12], matrix[13], matrix[14]);
			Quat rotation;

			// we will ignore the Rotation part of matrix and use the quaternion rotation stored in the body
			NewtonBodyGetRotation(body, (float*)&rotation);

			tBody * me = (tBody *)NewtonBodyGetUserData(body);

			SceneNode * node = me->GetSceneNode();

			if (node)
			{
				node->SetPosition(posit);
				node->SetOrientation(rotation);
			}

			Mat4 worldTm = *(Mat4 *)matrix;

			me->OnTransform(&posit, &rotation);
		}


		// callback to apply external forces to body
		void ApplyForceAndTorqueCallback (const NewtonBody* body, dFloat timestep, int threadIndex)
		{
			tBody * me = (tBody *)NewtonBodyGetUserData(body);

			Vec3 force = Vec3(0.0f, GRAVITY, 0.0f);

			me->OnApplyForce(&force, NULL);

			NewtonBodySetForce(body, &force[0]);
		}







		tRigidBody::tRigidBody(tShape * shape, SceneNode * node, float mass)
			: tBody(shape, tBody::eRigidBody)
		{
			mNode = node;
			Vec3 origin, inertia;

			Mat4 worldTm = Mat4::Identity;

			if (mNode)
			{
				worldTm.MakeTransform(node->GetWorldPosition(), node->GetWorldOrientation(), Vec3::Unit);
			}

			mBody = NewtonCreateBody(tWorld::Instance()->_getNewtonWorld(), shape->_getNewtonCollision(), worldTm[0]);

			NewtonBodySetDestructorCallback (mBody, DestroyBodyCallback);
			NewtonBodySetUserData(mBody, this);

			// we need to set the proper center of mass and inertia matrix for this body
			// the inertia matrix calculated by this function does not include the mass.
			// therefore it needs to be multiplied by the mass of the body before it is used.
			NewtonConvexCollisionCalculateInertialMatrix(shape->_getNewtonCollision(), &inertia[0], &origin[0]);	

			// set the body mass matrix
			NewtonBodySetMassMatrix(mBody, mass, mass * inertia.x, mass * inertia.y, mass * inertia.z);

			// set the body origin
			NewtonBodySetCentreOfMass(mBody, &origin[0]);

			// set the function callback to apply the external forces and torque to the body
			// the most common force is Gravity
			NewtonBodySetForceAndTorqueCallback(mBody, ApplyForceAndTorqueCallback);

			// set the function callback to set the transformation state of the graphic entity associated with this body 
			// each time the body change position and orientation in the physics world
			NewtonBodySetTransformCallback(mBody, SetTransformCallback);
		}
	}
}