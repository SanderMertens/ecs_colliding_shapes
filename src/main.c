#include <ecs_colliding_shapes.h>

#define THREADS (12)
#define NUM_SHAPES (4000)

#define SPEED (10)
#define SIZE (2.0)
#define FORCE (50000)
#define IMPLODE_FORCE (50000)
#define IMPLODE_REPEL_FORCE (3000)
#define IMPLODE_DISTANCE (30.0)
#define WALL_FORCE (300)

#define SCREEN_X (800)
#define SCREEN_Y (600)
#define MAX_X (SCREEN_X / 2)
#define MAX_Y (SCREEN_Y / 2)

void InitPV(ecs_rows_t *rows) {
    EcsPosition2D *p = ecs_column(rows, EcsPosition2D, 1);
    EcsVelocity2D *v = ecs_column(rows, EcsVelocity2D, 2);

    for (int i = 0; i < rows->count; i ++) {
        v[i].x = rand() % 2 - 1;
        v[i].y = rand() % 2 - 1;
        if (!v[i].x) v[i].x = 1;
        if (!v[i].y) v[i].y = 1;
        ecs_vec2_normalize(&v[i], &v[i]);
        ecs_vec2_mult(&v[i], rand() % SPEED, &v[i]);

        p[i].x = (rand() % (int)(SCREEN_X - SIZE)) + SIZE / 2 - MAX_X;
        p[i].y = (rand() % (int)(SCREEN_Y - SIZE)) + SIZE / 2 - MAX_Y;
    }
}

void Bounce(ecs_rows_t *rows) {
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

void FadeColor(ecs_rows_t *rows) {
    EcsColor *color = ecs_column(rows, EcsColor, 1);
    EcsVelocity2D *v = ecs_column(rows, EcsVelocity2D, 2);
    for (int i = 0; i < rows->count; i ++) {
        float speed = ecs_vec2_magnitude(&v[i]);
        float green = 50.0 * (speed / (float)SPEED);
        if (green > 255) {
            green = 255;
        }

        color[i] = (EcsColor){
            .r = color[i].r * 0.96, .g = green, .b = 255, .a = 255
        };
    }
}

void FadeVelocity(ecs_rows_t *rows) {
    EcsPosition2D *p = ecs_column(rows, EcsPosition2D, 1);
    EcsVelocity2D *v = ecs_column(rows, EcsVelocity2D, 2);

    for (int i = 0; i < rows->count; i ++) {
        float d_left = p[i].x - -SCREEN_X;
        float d_right = p[i].x - SCREEN_X;
        float d_up = p[i].y - -SCREEN_Y;
        float d_down = p[i].y - SCREEN_Y;
        d_left *= d_left;
        d_right *= d_right;
        d_up *= d_up;
        d_down *= d_down;

        v[i].x += WALL_FORCE / d_left - WALL_FORCE / d_right;
        v[i].y += WALL_FORCE / d_up - WALL_FORCE / d_down;
        
        float size = ecs_vec2_magnitude(&v[i]);
        if (size > SPEED) {
            ecs_vec2_mult(&v[i], 0.80, &v[i]);
        }
    }
}

void Explode(ecs_rows_t *rows) {
    EcsPosition2D *p = ecs_column(rows, EcsPosition2D, 1);
    EcsVelocity2D *v = ecs_column(rows, EcsVelocity2D, 2);
    EcsVec2 force = {0, 0};
    EcsVec2 *blast = rows->param;

    for (int i = 0; i < rows->count; i ++) {
        ecs_vec2_sub(&p[i], blast, &force);
        float distance = ecs_vec2_magnitude(&force);
        ecs_vec2_div(&force, (distance * distance * distance) / FORCE, &force);
        ecs_vec2_add(&v[i], &force, &v[i]);
    }    
}

void Implode(ecs_rows_t *rows) {
    EcsPosition2D *p = ecs_column(rows, EcsPosition2D, 1);
    EcsVelocity2D *v = ecs_column(rows, EcsVelocity2D, 2);
    EcsVec2 force = {0, 0};
    EcsVec2 *blast = rows->param;

    for (int i = 0; i < rows->count; i ++) {
        ecs_vec2_sub(&p[i], blast, &force);
        float distance = ecs_vec2_magnitude(&force);
        if (distance > IMPLODE_DISTANCE) {
            ecs_vec2_div(&force, (distance * distance * distance) / IMPLODE_FORCE, &force);
            ecs_vec2_sub(&v[i], &force, &v[i]);
        } else {
            ecs_vec2_div(&force, (distance * distance * distance) / IMPLODE_REPEL_FORCE, &force);
            ecs_vec2_add(&v[i], &force, &v[i]);
        }
    }    
}

void Input(ecs_rows_t *rows) {
    EcsInput *input = ecs_column(rows, EcsInput, 1);
    ecs_entity_t Explode = ecs_column_entity(rows, 2);
    ecs_entity_t Implode = ecs_column_entity(rows, 3);

    if (input->mouse.left.current) {
        ecs_run(rows->world, Explode, rows->delta_time, &input->mouse.view);
    } else if (input->mouse.right.current) {
        ecs_run(rows->world, Implode, rows->delta_time, &input->mouse.view);
    }
}

void OnCollide(ecs_rows_t *rows) {
    EcsCollision2D *collision = ecs_column(rows, EcsCollision2D, 1);
    ecs_type_t TEcsColor = ecs_column_type(rows, 2);

    for (int i = 0; i < rows->count; i ++) {
        EcsColor *c_1 = ecs_get_ptr(rows->world, collision[i].entity_1, EcsColor);
        EcsColor *c_2 = ecs_get_ptr(rows->world, collision[i].entity_2, EcsColor);

        c_1->r = 255;
        c_2->r = 255;
    }
}

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init_w_args(argc, argv);

    ECS_IMPORT(world, FlecsComponentsTransform, ECS_2D);
    ECS_IMPORT(world, FlecsComponentsGeometry, ECS_2D);
    ECS_IMPORT(world, FlecsComponentsGraphics, ECS_2D);
    ECS_IMPORT(world, FlecsSystemsPhysics, ECS_2D);
    ECS_IMPORT(world, FlecsSystemsSdl2, ECS_2D);

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

    /* Bounce shapes of screen boundaries */
    ECS_SYSTEM(world, Bounce, EcsOnUpdate, EcsPosition2D, EcsVelocity2D);

    /* Set color & velocity */
    ECS_SYSTEM(world, FadeColor, EcsOnUpdate, EcsColor, EcsVelocity2D);
    ECS_SYSTEM(world, FadeVelocity, EcsOnUpdate, EcsPosition2D, EcsVelocity2D);
    ECS_SYSTEM(world, OnCollide, EcsPostUpdate, EcsCollision2D, ID.EcsColor);

    /* Explode */
    ECS_SYSTEM(world, Explode, EcsManual, EcsPosition2D, EcsVelocity2D);
    ECS_SYSTEM(world, Implode, EcsManual, EcsPosition2D, EcsVelocity2D);
    ECS_SYSTEM(world, Input, EcsOnUpdate, EcsInput, ID.Explode, ID.Implode);

    /* Initialize canvas */
    ecs_set_singleton(world, EcsCanvas2D, {
        .window = { .width = SCREEN_X, .height = SCREEN_Y },
        .viewport = { .width = SCREEN_X, .height = SCREEN_Y, .x = -SCREEN_X / 2, .y = -SCREEN_Y / 2},
        .title = "Hello ecs_shapes!" 
    });

    /* Initialize shapes */
    ecs_new_w_count(world, Circle, NUM_SHAPES);

    ecs_set_threads(world, THREADS);

    /* Enter main loop */
    ecs_set_target_fps(world, 60);
    while ( ecs_progress(world, 0)) { };

    /* Cleanup */
    return ecs_fini(world);
}
