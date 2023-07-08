#include "boundsphere.h"
#include "model/nifmodel.h"

BoundSphere::BoundSphere()
{
	radius = -1;
}

BoundSphere::BoundSphere( const Vector3 & c, float r )
{
	center = c;
	radius = r;
}

BoundSphere::BoundSphere( const BoundSphere & other )
{
	operator=( other );
}

BoundSphere::BoundSphere( const NifModel * nif, const QModelIndex & index )
{
	auto idx = index;
	auto sph = nif->getIndex( idx, "Bounding Sphere" );
	if ( sph.isValid() )
		idx = sph;

	center = nif->get<Vector3>( idx, "Center" );
	radius = nif->get<float>( idx, "Radius" );
}

BoundSphere::BoundSphere( const QVector<Vector3> & verts )
{
	if ( verts.isEmpty() ) {
		center = Vector3();
		radius = -1;
	} else {
		center = Vector3();
		for ( const Vector3 & v : verts ) {
			center += v;
		}
		center /= verts.count();

		radius = 0;
		for ( const Vector3 & v : verts ) {
			float d = ( center - v ).squaredLength();

			if ( d > radius )
				radius = d;
		}
		radius = sqrt( radius );
	}
}

void BoundSphere::update( NifModel * nif, const QModelIndex & index )
{
	auto idx = index;
	QModelIndex sph;

	if ( nif->inherits( idx, "NiTriShape" ) )
		sph = nif->getIndex( nif->getBlock( nif->getLink( idx, "Data" ) ), "Bounding Sphere" );
	else
		sph = nif->getIndex( idx, "Bounding Sphere" );

	if ( sph.isValid() )
		idx = sph;

	nif->set<Vector3>( idx, "Center", center );
	nif->set<float>( idx, "Radius", radius );
}

void BoundSphere::setBounds( NifModel * nif, const QModelIndex & index, const Vector3 & center, float radius )
{
	BoundSphere( center, radius ).update( nif, index );
}

BoundSphere & BoundSphere::operator=( const BoundSphere & o )
{
	center = o.center;
	radius = o.radius;
	return *this;
}

BoundSphere & BoundSphere::operator|=( const BoundSphere & o )
{
	if ( o.radius < 0 )
		return *this;

	if ( radius < 0 )
		return operator=( o );

	float d = ( center - o.center ).length();

	if ( radius >= d + o.radius )
		return *this;

	if ( o.radius >= d + radius )
		return operator=( o );

	if ( o.radius > radius )
		radius = o.radius;

	radius += d / 2;
	center = ( center + o.center ) / 2;

	return *this;
}

BoundSphere BoundSphere::operator|( const BoundSphere & other )
{
	BoundSphere b( *this );
	b |= other;
	return b;
}

BoundSphere & BoundSphere::apply( const Transform & t )
{
	center = t * center;
	radius *= fabs( t.scale );
	return *this;
}

BoundSphere & BoundSphere::applyInv( const Transform & t )
{
	center = t.rotation.inverted() * ( center - t.translation ) / t.scale;
	radius /= fabs( t.scale );
	return *this;
}

BoundSphere operator*( const Transform & t, const BoundSphere & sphere )
{
	BoundSphere bs( sphere );
	return bs.apply( t );
}