// BspLineCollision.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"

#define DotProduct(x,y)			((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])

typedef unsigned char byte;
typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];

typedef struct cplane_s {
	vec3_t normal;
	float dist;
} cplane_t;

typedef struct {
	cplane_t *plane;
	int children[2];	// negative numbers are leafs
} cNode_t;

typedef struct {
	byte opaque;
} cLeaf_t;

typedef struct {
	float fraction;
} trace_t;

cLeaf_t leafs[] =
{
	{0},	// non opaque
	{1},	// opaque
};

cplane_t planes[] = {
	{ { 0.0f, -1.0f, 0.0f}, -10.0f }, // A (0)
	{ {-1.0f, 0.0f, 0.0f}, 50.0f }, // B (1)
	{ { 0.0f, 1.0f, 0.0f}, 15.0f }, // C (2)
	{ { 1.0f, 0.0f, 0.0f}, 15.0f }, // D (3)
	{ {-1.0f, 0.0f, 0.0f}, -10.0f }, // E (4)
	{ { 1.0f, 0.0f, 0.0f}, 15.0f }, // F ( same as D ) (5)
	{ { 0.0f,-1.0f, 0.0f}, 50.0f }, // G (6)
	{ { 1.0f, 0.0f, 0.0f}, 0.0f }, // H (7)
	{ { 0.0f, 1.0f, 0.0f}, 0.0f }, // I (8)
	{ {-1.0f, 0.0f, 0.0f}, 50.0f }, // J (9)
	{ { 0.0f, -1.0f, 0.0f}, 5.0f }, // K (10)
	{ {-1.0f, 0.0f, 0.0f}, 5.0f }, // L (11)
	{ { 0.0f, -1.0f, 0.0f}, 50.0f }, // M (12)
};

// empty leaf = -1
// opaque leaf = -2
cNode_t nodes[] =
{
	{ planes + 0, {4, 1} }, // A
	{ planes + 1, {-1, 2} }, // B
	{ planes + 2, {-1, 3} }, // C
	{ planes + 3, {-1, -2} }, // D
	{ planes + 4, {7, 5} }, // E
	{ planes + 5, {-1, 6} }, // F
	{ planes + 6, {-1, -2} }, // G
	{ planes + 7, {-1, 8} }, // H
	{ planes + 8, {-1, 9} },// I
	{ planes + 9, {-1, 10} }, // J
	{ planes + 10, {11, -2} }, // K
	{ planes + 11, {-1, 12} }, // L
	{ planes + 12, { -1, -2} },// M
};



void PrintScene()
{
	// planes
	const int num_nodes = sizeof(nodes)/sizeof(nodes[0]);
	cNode_t* node = nodes;
	for( int i = 0; i < num_nodes; ++i)
	{
		cplane_t* p = node->plane;
		
		int num = node->children[0];
		if (num < 0)
		{
			cLeaf_t* leaf = leafs + (-1 - num);
			printf("%s + ", leaf->opaque ? "solid" : "leaf");
		}
		else
		{
			printf("node(%c) + ", 'A' + num);
		}

		printf("node(%c)", (char)('A' + i));

		num = node->children[1];
		if (num < 0)
		{
			cLeaf_t* leaf = leafs + (-1 - num);
			printf(" - %s :", leaf->opaque ? "solid" : "empty");
		}
		else
		{
			printf(" - node(%c) :", 'A' + num);
		}
		printf("plane = %2.2f, %2.2f, %2.2f, %2.2f\n", p->normal[0], p->normal[1], p->normal[2], p->dist);

		node++;
	}
}

void TraceThroughTree( trace_t* t, int num, float p1f, float p2f, vec3_t p1, vec3_t p2) {

	if (t->fraction <= p1f) {
		return; // already hit something nearer
	}

	// if <0, we are in a leaf node
	if (num < 0) {
		cLeaf_t& leaf = leafs[-1-num];
		if (leaf.opaque) {
			t->fraction = p1f;
		}
		return;
	}

	//
	// find the point distances to the seperating plane
	cNode_t* node = nodes + num;
	cplane_t* plane = node->plane;
	float t1 = DotProduct(plane->normal, p1) - plane->dist;
	float t2 = DotProduct(plane->normal, p2) - plane->dist;

	// see which sides we need to consider
	if ( t1 >= 1 && t2 >= 1 ) {
		TraceThroughTree( t, node->children[0], p1f, p2f, p1, p2 );
		return;
	}
	if ( t1 < -1 && t2 < -1 ) {
		TraceThroughTree( t, node->children[1], p1f, p2f, p1, p2 );
		return;
	}

	// put the crosspoint on the near side
	float frac, frac2;
	float idist;
	int side;
	if ( t1 < t2 ) {
		idist = 1.0f /(t1-t2);
		side = 1;
		frac = frac2 = t1 * idist;
	} else if ( t1 > t2 ) {
		idist = 1.0f / (t1-t2);
		side = 0;
		frac = frac2 = t1 * idist;
	} else {
		side = 0;
		frac = 1.0f;
		frac2 = 0.0f;
	}

	// move up to the node
	if ( frac < 0.0f ) {
		frac = 0.0f;
	}
	else if ( frac > 1.0f ) {
		frac = 1.0f;
	}

	float midf = p1f + (p2f - p1f) * frac;
	vec3_t mid;

	mid[0] = p1[0] + frac*(p2[0] - p1[0]);
	mid[1] = p1[1] + frac*(p2[1] - p1[1]);
	mid[2] = p1[2] + frac*(p2[2] - p1[2]);

	TraceThroughTree( t, node->children[side], p1f, midf, p1, mid );

	// go pat the node
	if ( frac2 < 0.0f ) {
		frac2 = 0.0f;
	} else if ( frac2 > 1.0f ) {
		frac2 = 1.0f;
	}

	midf = p1f + (p2f - p1f)*frac2;

	mid[0] = p1[0] + frac2*(p2[0] - p1[0]);
	mid[1] = p1[1] + frac2*(p2[1] - p1[1]);
	mid[2] = p1[2] + frac2*(p2[2] - p1[2]);

	TraceThroughTree( t, node->children[side^1], midf, p2f, mid, p2 );

}

// Non Recursive Version
// History Flags
typedef struct cstackdata_s {
	int node_num;
	vec3_t p1;
	float p1f;
	vec3_t p2;
	float p2f;
} cstackdata_t;

cstackdata_t stack[64];

void TraceThroughTree2( trace_t* t, float p1f, float p2f, vec3_t start, vec3_t end) {

	int stackptr = -1;
	int num = 0;
	vec3_t p1;
	vec3_t p2;
	p1[0] = start[0];
	p1[1] = start[1];
	p1[2] = start[2];
	p2[0] = end[0];
	p2[1] = end[1];
	p2[2] = end[2];

	while(1) {

		// if <0, we are in a leaf node
		if (num < 0) {
			cLeaf_t& leaf = leafs[-1-num];
			if (leaf.opaque) {
				t->fraction = t->fraction < p1f ? t->fraction : p1f;
			}

			if( stackptr < 0 ) {
				break;
			}

			// pop node
			num = stack[stackptr].node_num;

			p1[0] = stack[stackptr].p1[0];
			p1[1] = stack[stackptr].p1[1];
			p1[2] = stack[stackptr].p1[2];
			p1f = stack[stackptr].p1f;

			p2[0] = stack[stackptr].p2[0];
			p2[1] = stack[stackptr].p2[1];
			p2[2] = stack[stackptr].p2[2];
			p2f = stack[stackptr].p2f;
			
			stackptr--;

			continue;
		}

		cNode_t* node = nodes + num;
		cplane_t* plane = node->plane;
		float t1 = DotProduct(plane->normal, p1) - plane->dist;
		float t2 = DotProduct(plane->normal, p2) - plane->dist;

		// see which sides we need to consider
		if ( t1 >= 1 && t2 >= 1 ) {
			num = node->children[0];
			continue;
		}

		if ( t1 < -1 && t2 < -1 ) {
			num = node->children[1];
			continue;
		}

		// put the crosspoint on the near side
		float frac, frac2;
		float idist;
		int side;
		if ( t1 < t2 ) {
			idist = 1.0f /(t1-t2);
			side = 1;
			frac = frac2 = t1 * idist;

		} else if ( t1 > t2 ) {
			idist = 1.0f / (t1-t2);
			side = 0;
			frac = frac2 = t1 * idist;

		} else {
			side = 0;
			frac = 1.0f;
			frac2 = 0.0f;

		}

		// - child 노드를 stack에 넣어 놓는다.
		// go past the node
		if ( frac2 < 0.0f ) {
			frac2 = 0.0f;
		} else if ( frac2 > 1.0f ) {
			frac2 = 1.0f;
		}

		float midf = p1f + (p2f - p1f)*frac2;
		vec3_t mid;
		mid[0] = p1[0] + frac2*(p2[0] - p1[0]);
		mid[1] = p1[1] + frac2*(p2[1] - p1[1]);
		mid[2] = p1[2] + frac2*(p2[2] - p1[2]);

		stackptr++;
		stack[stackptr].node_num = node->children[side^1];
		stack[stackptr].p1[0] = mid[0];
		stack[stackptr].p1[1] = mid[1];
		stack[stackptr].p1[2] = mid[2];
		stack[stackptr].p1f = midf;
		stack[stackptr].p2[0] = p2[0];
		stack[stackptr].p2[1] = p2[1];
		stack[stackptr].p2[2] = p2[2];
		stack[stackptr].p2f = p2f;

		// 이 노드에 의해서 잘린 경우,
		// + child 노드를 다음 처리 노드로 설정한다.
		// move up to the node
		if ( frac < 0.0f ) {
			frac = 0.0f;
		}
		else if ( frac > 1.0f ) {
			frac = 1.0f;
		}

		midf = p1f + (p2f - p1f) * frac;
		
		mid[0] = p1[0] + frac*(p2[0] - p1[0]);
		mid[1] = p1[1] + frac*(p2[1] - p1[1]);
		mid[2] = p1[2] + frac*(p2[2] - p1[2]);

		num = node->children[side];
		p2f = midf;
		p2[0] = mid[0];
		p2[1] = mid[1];
		p2[2] = mid[2];
	}
}

typedef struct traceCase_s
{
	vec3_t p1;
	vec3_t p2;
} traceCast_t;

traceCast_t traces[] = {
	{ {0.0f, 2.0f, 0.0f}, {0.0f, 20.0f, 0.0f} },
	{ {-10.0f, -10.0f, 0.0f}, { 20.0f, 20.0f, 0.0f} },
	{ {20.0f, 20.0f, 0.0f}, { -10.0f, -10.0f, 0.0f} },
};


int _tmain(int argc, _TCHAR* argv[])
{
	PrintScene();

	const int num_trace = sizeof(traces) / sizeof(traces[0]);

	traceCast_t* tc = traces;
	for( int i = 0; i < num_trace; ++i)
	{
		trace_t t;
		t.fraction = 1.0f;

		trace_t t2;
		t2.fraction = 1.0f;

		TraceThroughTree(&t, 0, 0.0f, 1.0f, tc->p1, tc->p2);
		TraceThroughTree2(&t2, 0.0f, 1.0f, tc->p1, tc->p2);

		vec3_t hitpos;
		hitpos[0] = tc->p1[0] + t.fraction*(tc->p2[0] - tc->p1[0]);
		hitpos[1] = tc->p1[1] + t.fraction*(tc->p2[1] - tc->p1[1]);
		hitpos[2] = tc->p1[2] + t.fraction*(tc->p2[2] - tc->p1[2]);

		printf("  Trace %d: p1(%2.2f,%2.2f,%2.2f), p2(%2.2f,%2.2f,%2.2f) = %1.3f(%2.2f, %2.2f, %2.2f)\n",
			i,
			tc->p1[0], tc->p1[1], tc->p1[2],
			tc->p2[0], tc->p2[1], tc->p1[2],
			t.fraction,
			hitpos[0], hitpos[1], hitpos[2]);

		if ( t.fraction != t2.fraction )
		{
			vec3_t hitpos;
			hitpos[0] = tc->p1[0] + t2.fraction*(tc->p2[0] - tc->p1[0]);
			hitpos[1] = tc->p1[1] + t2.fraction*(tc->p2[1] - tc->p1[1]);
			hitpos[2] = tc->p1[2] + t2.fraction*(tc->p2[2] - tc->p1[2]);
			printf("  Diff Trace2 %d: p1(%2.2f,%2.2f,%2.2f), p2(%2.2f,%2.2f,%2.2f) = %1.3f(%2.2f, %2.2f, %2.2f)\n",
				i,
				tc->p1[0], tc->p1[1], tc->p1[2],
				tc->p2[0], tc->p2[1], tc->p1[2],
				t2.fraction,
				hitpos[0], hitpos[1], hitpos[2]);
		}
		else
		{
			printf("  Same Trace %d. Good Job~\n", i);
		}

		tc++;
	}
	return 0;
}

