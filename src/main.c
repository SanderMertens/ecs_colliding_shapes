#include <include/ecs_colliding_shapes.h>

#define NUM_SHAPES (1000)
#define SCREEN_X (800)
#define SCREEN_Y (600)
#define MAX_X (SCREEN_X / 2)
#define MAX_Y (SCREEN_Y / 2)
#define SPEED (40)
#define SIZE (7)

void InitPV(EcsRows *rows) {
    EcsPosition2D *p = ecs_column(rows, EcsPosition2D, 1);
    EcsVelocity2D *v = ecs_column(rows, EcsVelocity2D, 2);

    for (int i = 0; i < rows->count; i ++) {
        v[i].x = rand() % SPEED - SPEED / 2;
        v[i].y = rand() % SPEED - SPEED / 2;
        p[i].x = (rand() % (int)(SCREEN_X - SIZE)) + SIZE / 2 - MAX_X;
        p[i].y = (rand() % (int)(SCREEN_Y - SIZE)) + SIZE / 2 - MAX_Y;
    }
}

void Bounce(EcsRows *rows) {
    EcsPosition2D *p = ecs_column(rows, EcsPosition2D, 1);
    EcsVelocity2D *v = ecs_column(rows, EcsVelocity2D, 2);

    for (int i = 0; i < rows->count; i ++) {
        if (ecs_clamp(&p[i].x, -MAX_X + SIZE / 2, MAX_X - SIZE / 2)) {
            v[i].x *= -1.0; /* Reverse x velocity if ball hits a vertical wall */
        }

        if (ecs_clamp(&p[i].y, -MAX_Y + SIZE / 2, MAX_Y - SIZE / 2)) {
            v[i].y *= -1.0; /* Reverse x velocity if ball hits a vertical wall */
        }
    }
}

void ResetColor(EcsRows *rows) {
    EcsColor *color = ecs_column(rows, EcsColor, 1);
    for (int i = 0; i < rows->count; i ++) {
        color[i] = (EcsColor){
            .r = color[i].r * 0.97, .g = 50, .b = 255, .a = 255
        };
    }
}

void SetColorOnCollide(EcsRows *rows) {
    EcsCollision2D *collision = ecs_column(rows, EcsCollision2D, 1);
    EcsType TEcsColor = ecs_column_type(rows, 2);

    for (int i = 0; i < rows->count; i ++) {
        ecs_set(rows->world, collision[i].entity_1, EcsColor, {255, 50, 100, 255});
        ecs_set(rows->world, collision[i].entity_2, EcsColor, {255, 50, 100, 255});
    }
}

int main(int argc, char *argv[]) {
    EcsWorld *world = ecs_init();

    ECS_IMPORT(world, EcsComponentsTransform, ECS_2D);
    ECS_IMPORT(world, EcsComponentsGeometry, ECS_2D);
    ECS_IMPORT(world, EcsComponentsGraphics, ECS_2D);
    ECS_IMPORT(world, EcsSystemsPhysics, ECS_2D);
    ECS_IMPORT(world, EcsSystemsSdl2, ECS_2D);

    /* Define prefabs for Circle and Square (for shared components) */
    ECS_PREFAB(world, ShapePrefab, EcsCollider, EcsColor);
    ECS_PREFAB(world, SquarePrefab, ShapePrefab, EcsSquare);
    ECS_PREFAB(world, CirclePrefab, ShapePrefab, EcsCircle);
    ecs_set(world, ShapePrefab, EcsColor, {.r = 0, .g = 50, .b = 100, .a = 255});
    ecs_set(world, SquarePrefab, EcsSquare, {.size = SIZE});
    ecs_set(world, CirclePrefab, EcsCircle, {.radius = SIZE / 2});

    /* Shape type (for owned components, override color) */
    ECS_TYPE(world, Shape, EcsPosition2D, EcsVelocity2D);

    /* Combine shared & owned components into dedicated Square & Circle types */
    ECS_TYPE(world, Square, SquarePrefab, Shape, EcsColor);
    ECS_TYPE(world, Circle, CirclePrefab, Shape, EcsColor);

    /* Initialization system with random position & velocity */
    ECS_SYSTEM(world, InitPV, EcsOnAdd, EcsPosition2D, EcsVelocity2D);
    ECS_SYSTEM(world, Bounce, EcsOnFrame, EcsPosition2D, EcsVelocity2D);
    ECS_SYSTEM(world, ResetColor, EcsOnFrame, EcsColor);
    ECS_SYSTEM(world, SetColorOnCollide, EcsOnSet, EcsCollision2D, ID.EcsColor);

    /* Initialize canvas */
    ecs_set_singleton(world, EcsCanvas2D, {
        .window = { .width = SCREEN_X, .height = SCREEN_Y }, 
        .title = "Hello ecs_shapes!" 
    });

    /* Initialize shapes */
    ecs_new_w_count(world, Square, NUM_SHAPES / 2, NULL);
    ecs_new_w_count(world, Circle, NUM_SHAPES / 2, NULL);

    /* Enter main loop */
    ecs_set_target_fps(world, 60);
    while ( ecs_progress(world, 0)) { };

    /* Cleanup */
    return ecs_fini(world);
}
