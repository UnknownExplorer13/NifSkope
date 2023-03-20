/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#ifndef GLTOOLS_H
#define GLTOOLS_H

#include "data/niftypes.h"
#include "gltools/boundsphere.h"
#include "gltools/boneweights.h"
#include "gltools/skinpartition.h"

#include <QOpenGLContext>



//! @file gltools.h BoundSphere, VertexWeight, BoneWeights, SkinPartition

namespace BKHUtils
{
	/*!
	 * @brief Determines the base scale ratio for bhk objects
	 * @param nif The model containing the shapes
	 * @return The base scale factor for bhk objects
	*/
	float bhkScale( const NifModel * nif );
	
	/*!
	 * @brief Determines the inverted base scale ratio for bhk objects
	 * @param nif The model containing the shapes
	 * @return The inverted base scale factor (2 = 0.5)
	*/
	float bhkInvScale( const NifModel * nif );

	/*!
	 * @brief Determines the default scale multiplier for bhk objects
	 * @param nif The model containing the shapes
	 * @return The Scale to use
	*/
	float bhkScaleMult( const NifModel * nif );

	/*!
	 * @brief Determines the overall transform values for a bhk object
	 * @param nif   The model containing the object
	 * @param index A pointer to the object you want to know the transform of
	 * @return The overall transformation of the bhk object
	*/
	Transform bhkBodyTrans( const NifModel * nif, const QModelIndex & index );


	QModelIndex bhkGetEntity( const NifModel * nif, const QModelIndex & index, const QString & name );
	QModelIndex bhkGetRBInfo( const NifModel * nif, const QModelIndex & index, const QString & name );
}

namespace GLUtils
{
	struct CompoundId
	{
		enum class Type
		{
			//! When referencing a blocks without selecting one of it's children.
			Object = 0x0,
			//! When referencing vertices in a block.
			Vertex = 0x1,
			//! When referencing a furniture marker in a block.
			FurnitureMarker = 0x2,
			//! When you are referencing a connect point in a block.
			ConnectPoint = 0x3,
			//! Nothing will be selected 
			None = 0xF
		};

		//! The block number of the selected item
		int blockNumber;
		//! The type of selection
		int pos;
		//! The type of thing to select
		CompoundId::Type type;

		CompoundId( const int compoundId )
		{
			int actual = compoundId - 1;
			blockNumber = actual & 0xFFF;
			pos = ( actual >> 12 ) & 0xFFFF;
			type = ( CompoundId::Type ) (( actual >> 28 ) & 0xF);
		}

		/*!
		 * @brief Creates a compound id used to select blocks or items withing a block.
		 * @param blockNumber The block number of the index.
		 * @param pos         The position of the item.
		 * @param type        The type of storage location where we can find the selected object.
		 * @return
		 */
		static int serialize( const int blockNumber, const int pos = 0, const Type type = Type::Object )
		{
			return (
				// caps at 4096, (i haven't seen a file that goes over that so I'm assuming it's safe)
				( blockNumber & 0xFFF ) |
				// caps at 65536, vertex arrays can be pretty large. Though if your mesh has that many verticies, i would recommend you simplify your mesh.
				( ( pos & 0xFFFF ) << 12 ) |
				// caps at 16 (may need to expand but it's good enough for now)
				( ( ( int ) type & 0xF ) << 28 )
			) + 1;
		}
	};

	QVector<int> sortAxes( QVector<float> axesDots );

	/*!
	 * @brief Draws an from a to b.
	 * @param pos         The center of the arc.
	 * @param startPoint  Where to start the arc from.
	 * @param endPoint    Where to end the arc at.
	 * @param startAngle  Where to start drawing the arc.
	 * @param endAngle    Where to stop drawing the arc.
	 * @param numSegments The number of segments forming the arc.
	*/
	void drawArc( const Vector3 & pos, const Vector3 & startPoint, const Vector3 & endPoint, float startAngle, float endAngle, int numSegments = 8);

	/*!
	 * @brief Draws an arrow axis.
	 * @param pos   Where to draw the axis
	 * @param scale How big to make the axis.
	 * @param useRGB True if you want rgb axes.
	*/
	void drawAxes( const Vector3 & pos, float scale, bool useRGB = true );

	/*!
	 * @brief Draws the axes used for the overlay in the bottom left.
	 * @param pos       The position of the axes.
	 * @param scale     The scale of the axes.
	 * @param axesOrder The order in which to render the axes.
	*/
	void drawAxesOverlay( const Vector3 & pos, float scale, QVector<int> axesOrder = { 2, 1, 0 } );

	/*!
	 * @brief Draws a box between p1 an p2.
	 * @param p1 The first point.
	 * @param p2 The second point.
	*/
	void drawBox( const Vector3 & p1, const Vector3 & p2 );
	
	/*!
	 * @brief Draws a capsule of a specific radius between p1 and p2.
	 * @param p1          The first point.
	 * @param p2          The second point.
	 * @param radius      The radius of the capsule.
	 * @param numSegments The number of segments to use.
	*/
	void drawCapsule( const Vector3 & p1, const Vector3 & p2, float radius, int numSegments = 5 );
	
	/*!
	 * @brief Draws a cylinder of a specific radius between p1 and p2.
	 * @param p1          The front side of the cylinder.
	 * @param p2          The rear side of the cylinder.
	 * @param radius      The radius of the cylinder.
	 * @param numSegments The number of segments to use.
	*/
    void drawCylinder( const Vector3 & p1, const Vector3 & p2, float radius, int numSegments = 5 );
	
	/*!
	 * @brief Draws a circle
	 * @param pos         The center of the circle.
	 * @param norm        The normalized direction of the circle.
	 * @param radius      The radius of the circle.
	 * @param numSegments The number of segments to use.
	*/
	void drawCircle( const Vector3 & pos, const Vector3 & norm, float radius, int numSegments = 16 );
	
	/*!
	 * @brief Draws a compressed mesh shape.
	 * @param nif      The model containing the shape.
	 * @param iShape   A pointer to the shape.
	 * @param isSolid  Wether to draw the shape as a solid.
	*/
	void drawCMS( const NifModel * nif, const QModelIndex & iShape, bool isSolid = false );

	/*!
	 * @brief Draws a cone
	 * @param base        The position of the Base of the cone.
	 * @param apex        The position of the apex of the cone.
	 * @param radius      The radius of the cone.
	 * @param numSegments The number of segments.
	*/
	void drawCone( const Vector3 & base, Vector3 apex, float radius, int numSegments );

	/*!
	 * @brief Draws the shape for connect points
	 * @param pos The position of the shape
	*/
	void drawConnectPoint( const Vector3 & pos);

	/*!
	 * @brief Draws a convex hull
	 * @param nif    The model containing the convex hull.
	 * @param iShape A pointer to the sape within the model.
	 * @param scale  The scale of the shape.
	 * @param solid  Wether to fill in the faces.
	*/
	void drawConvexHull( const NifModel * nif, const QModelIndex & iShape, float scale, bool solid = false );

	/*!
	 * @brief Draws a dashed line
	 * @param p1          The starting point of the line.
	 * @param p2          The end point of the line.
	 * @param numSegments The number of dashes to create.
	*/
	void drawDashLine( const Vector3 & p1, const Vector3 & p2, int numSegments = 15 );

	/*!
	 * @brief Draws a grid
	 * @param size         The size of the grid.
	 * @param spacing      The spacing between the lines.
	 * @param subdivisions The number of subdivisions
	*/
	void drawGrid( int size, int spacing, int subdivisions );

	void drawNiTSS( const NifModel * nif, const QModelIndex & iShape, bool solid = false );
	void drawRagdollCone( const Vector3 & pivot, const Vector3 & twist, const Vector3 & plane, float coneAngle, float minPlaneAngle, float maxPlaneAngle, int sd = 16 );
	void drawRail( const Vector3 & a, const Vector3 & b );
	void drawSolidArc( const Vector3 & c, const Vector3 & n, const Vector3 & x, const Vector3 & y, float an, float ax, float r, int sd = 8 );
	void drawSphereSimple( const Vector3 & c, float r, int sd = 36 );
	void drawSphere( const Vector3 & c, float r, int sd = 8 );
	void drawSpring( const Vector3 & a, const Vector3 & b, float stiffness, int sd = 16, bool solid = false );
	void drawText( double x, double y, double z, const QString & str );
	void drawText( const Vector3 & c, const QString & str );
}

/*
 * OpenGL Shorthands
 */

inline void glTranslate( const Vector3 & v )
{
	glTranslatef( v[ 0 ], v[ 1 ], v[ 2 ] );
}

inline void glScale( const Vector3 & v )
{
	glScalef( v[ 0 ], v[ 1 ], v[ 2 ] );
}

inline void glVertex( const Vector2 & v )
{
	glVertex2fv( v.data() );
}

inline void glVertex( const Vector3 & v )
{
	glVertex3fv( v.data() );
}

inline void glVertex( const Vector4 & v )
{
	glVertex3fv( v.data() );
}

inline void glNormal( const Vector3 & v )
{
	glNormal3fv( v.data() );
}

inline void glTexCoord( const Vector2 & v )
{
	glTexCoord2fv( v.data() );
}

inline void glColor( const Color3 & c )
{
	glColor3fv( c.data() );
}

inline void glColor( const Color4 & c )
{
	glColor4fv( c.data() );
}

inline void glMaterial( GLenum x, GLenum y, const Color4 & c )
{
	glMaterialfv( x, y, c.data() );
}

inline void glLoadMatrix( const Matrix4 & m )
{
	glLoadMatrixf( m.data() );
}

inline void glMultMatrix( const Matrix4 & m )
{
	glMultMatrixf( m.data() );
}

inline void glLoadMatrix( const Transform & t )
{
	glLoadMatrix( t.toMatrix4() );
}

inline void glMultMatrix( const Transform & t )
{
	glMultMatrix( t.toMatrix4() );
}

inline GLuint glClosestMatch( GLuint * buffer, GLint hits )
{
	// a little helper function, returns the closest matching hit from the name buffer
	GLuint choose = buffer[ 3 ];
	GLuint depth = buffer[ 1 ];

	for ( int loop = 1; loop < hits; loop++ ) {
		if ( buffer[ loop * 4 + 1 ] < depth ) {
			choose = buffer[ loop * 4 + 3 ];
			depth = buffer[ loop * 4 + 1 ];
		}
	}

	return choose;
}

#endif
