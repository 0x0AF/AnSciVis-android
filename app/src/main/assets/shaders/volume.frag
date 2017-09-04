#version 300 es
precision mediump float;
precision mediump sampler2D;
precision mediump sampler3D;
//#extension GL_ARB_shading_language_420pack : require
//#extension GL_ARB_explicit_attrib_location : require

#define TASK 10
#define ENABLE_OPACITY_CORRECTION 0
#define ENABLE_LIGHTNING 0
#define ENABLE_SHADOWING 0

in vec3 ray_entry_position;

layout(location = 0) out vec4 FragColor;

uniform mat4 Modelview;

uniform sampler3D volume_texture;
uniform sampler2D transfer_texture;

uniform vec3 camera_location;
uniform float sampling_distance;
uniform float sampling_distance_ref;
uniform float iso_value;
uniform vec3 max_bounds;
uniform ivec3 volume_dimensions;

uniform vec3 light_position;
uniform vec3 light_ambient_color;
uniform vec3 light_diffuse_color;
uniform vec3 light_specular_color;
uniform float light_ref_coef;

bool inside_volume_bounds(const in vec3 sampling_position) { return (all(greaterThanEqual(sampling_position, vec3(0.0))) && all(lessThanEqual(sampling_position, max_bounds))); }
float get_sample_data(vec3 in_sampling_pos)
{
    vec3 obj_to_tex = vec3(1.0) / max_bounds;
    return texture(volume_texture, in_sampling_pos * obj_to_tex).r;
}

vec3 get_gradient(vec3 pos)
{
    vec3 dx, dy, dz;

    dx = vec3(0.0001, 0.0, 0.0);
//    dx = vec3(max_bounds.x / volume_dimensions.x, 0.0, 0.0);
    dy = vec3(0.0, 0.0001, 0.0);
//    dy = vec3(0.0, max_bounds.y / volume_dimensions.y, 0.0);
    dz = vec3(0.0, 0.0, 0.0001);
//    dz = vec3(0.0, 0.0, max_bounds.z / volume_dimensions.z);

    float x_r = get_sample_data(pos + dx);
    float x_l = get_sample_data(pos - dx);
    float y_r = get_sample_data(pos + dy);
    float y_l = get_sample_data(pos - dy);
    float z_r = get_sample_data(pos + dz);
    float z_l = get_sample_data(pos - dz);

    return vec3((x_r - x_l) / 2.0, (y_r - y_l) / 2.0, (z_r - z_l) / 2.0);
}

void main()
{
    /// One step trough the volume
    vec3 ray_increment = normalize(ray_entry_position - camera_location) * sampling_distance;
    /// Position in Volume
    vec3 sampling_pos = ray_entry_position + ray_increment; // test, increment just to be sure we are in the volume

    /// Init color of fragment
    vec4 dst = vec4(0.0, 0.0, 0.0, 0.0);

    /// check if we are inside volume
    bool inside_volume = inside_volume_bounds(sampling_pos);

    if(!inside_volume)
        discard;

#if TASK == 10
    vec4 max_val = vec4(0.0, 0.0, 0.0, 0.0);

    // the traversal loop,
    // termination when the sampling position is outside volume boundarys
    // another termination condition for early ray termination is added
    while(inside_volume)
    {
        // get sample
        float s = get_sample_data(sampling_pos);

        // apply the transfer functions to retrieve color and opacity
        vec4 color = texture(transfer_texture, vec2(s, s));

        // this is the example for maximum intensity projection
        max_val.r = max(color.r, max_val.r);
        max_val.g = max(color.g, max_val.g);
        max_val.b = max(color.b, max_val.b);
        max_val.a = max(color.a, max_val.a);

        // increment the ray sampling position
        sampling_pos += ray_increment;

        // update the loop termination condition
        inside_volume = inside_volume_bounds(sampling_pos);
    }

    dst = max_val;
#endif

#if TASK == 11
    vec4 average_val = vec4(0.0, 0.0, 0.0, 0.0);
    float i = 0.0;

    // the traversal loop,
    // termination when the sampling position is outside volume boundarys
    // another termination condition for early ray termination is added
    while(inside_volume)
    {
        // get sample
        float s = get_sample_data(sampling_pos);

        vec4 color = texture(transfer_texture, vec2(s, s));

        if(i != 0.0)
        {
            i++;
            average_val.r = average_val.r + (color.r - average_val.r) / i;
            average_val.g = average_val.g + (color.g - average_val.g) / i;
            average_val.b = average_val.b + (color.b - average_val.b) / i;
            average_val.a = average_val.a + (color.a - average_val.a) / i;
        }
        else
        {
            i++;
            average_val.r = color.r;
            average_val.g = color.g;
            average_val.b = color.b;
            average_val.a = color.a;
        }

        // increment the ray sampling position
        sampling_pos += ray_increment;

        // update the loop termination condition
        inside_volume = inside_volume_bounds(sampling_pos);
    }

    dst = average_val;

#endif

#if TASK == 12 || TASK == 13
    // the traversal loop,
    // termination when the sampling position is outside volume boundarys
    // another termination condition for early ray termination is added
    while(inside_volume)
    {
        // get sample
        float s = get_sample_data(sampling_pos);

        if(s > iso_value)
        {
#if TASK == 13 // Binary Search

            vec3 hit_pos = sampling_pos;
            sampling_pos = sampling_pos - ray_increment;

            float epsilon = 0.001;

            while(length(hit_pos - ray_entry_position) - length(sampling_pos - ray_entry_position) > epsilon)
            {
                sampling_pos = sampling_pos + ((hit_pos - sampling_pos) / 2.0);
                s = get_sample_data(sampling_pos);

                if(s > iso_value)
                {
                    vec3 step = hit_pos - sampling_pos;
                    hit_pos = sampling_pos;
                    sampling_pos = hit_pos - step;
                }
            }

#endif

            dst = texture(transfer_texture, vec2(s, s));

#if ENABLE_LIGHTNING == 1 // Add Shading

            vec3 normal = normalize(-get_gradient(sampling_pos));
            vec3 lightDir = normalize(light_position - sampling_pos);

            float lambertian = max(dot(lightDir, normal), 0.0);
            float specular = 0.0;

            if(lambertian > 0.0)
            {
                vec3 viewDir = normalize(-sampling_pos);
                vec3 halfDir = normalize(lightDir + viewDir);
                float specAngle = max(dot(halfDir, normal), 0.0);
                specular = pow(specAngle, light_ref_coef);
            }

            vec3 colorLinear = light_ambient_color + lambertian * dst.xyz * light_diffuse_color + specular * light_specular_color;
            dst = vec4(colorLinear, dst.a);
//            dst = vec4(normal, dst.a);

#if ENABLE_SHADOWING == 1 // Add Shadows

            vec3 shadow_ray_increment = normalize(light_position - sampling_pos) * sampling_distance;
            vec3 shadow_sampling_pos = sampling_pos + shadow_ray_increment;
            bool shadow_ray_inside = inside_volume_bounds(shadow_sampling_pos);

            while(shadow_ray_inside)
            {
                float s = get_sample_data(shadow_sampling_pos);

                if(s > iso_value)
                {
                    dst = vec4(dst.xyz, 0.2);
                    break;
                }

                shadow_sampling_pos += shadow_ray_increment;
                shadow_ray_inside = inside_volume_bounds(shadow_sampling_pos);
            }
#endif
#endif

            break;
        }

        // increment the ray sampling position
        sampling_pos += ray_increment;

        // update the loop termination condition
        inside_volume = inside_volume_bounds(sampling_pos);
    }
#endif

#if TASK == 31
    // the traversal loop,
    // termination when the sampling position is outside volume boundarys
    // another termination condition for early ray termination is added
    vec3 comp_color = vec3(0, 0, 0);
    float comp_transp = 1.0;
    float i = 0.0;

    while(inside_volume)
    {
        // get sample

        float s = get_sample_data(sampling_pos);
        vec4 color = texture(transfer_texture, vec2(s, s));

#if ENABLE_LIGHTNING == 1 // Add Shading
        vec3 normal = normalize(-get_gradient(sampling_pos));
        vec3 lightDir = normalize(light_position - sampling_pos);

        float lambertian = max(dot(lightDir, normal), 0.0);
        float specular = 0.0;

        if(lambertian > 0.0)
        {
            vec3 viewDir = normalize(-sampling_pos);
            vec3 halfDir = normalize(lightDir + viewDir);
            float specAngle = max(dot(halfDir, normal), 0.0);
            specular = pow(specAngle, light_ref_coef);
        }

        color = vec4(light_ambient_color + lambertian * color.xyz * light_diffuse_color + specular * light_specular_color, color.a);
#endif

        // back to front
        //    if (i != 0) {
        //      i++;
        //      comp_color.r = comp_color.r * (1 - comp_transp) + color.a * color.r;
        //      comp_color.g = comp_color.g * (1 - comp_transp) + color.a * color.g;
        //      comp_color.b = comp_color.b * (1 - comp_transp) + color.a * color.b;
        //      comp_transp = (1 - color.a) * comp_transp;
        //    } else {
        //      i++;
        //      comp_color.r = color.r * color.a;
        //      comp_color.g = color.g * color.a;
        //      comp_color.b = color.b * color.a;
        //      comp_transp = 1 - color.a;
        //    }

        // front to back
        if(i != 0.0)
        {
            i++;
            comp_color.r = comp_color.r + color.a * color.r * comp_transp;
            comp_color.g = comp_color.g + color.a * color.g * comp_transp;
            comp_color.b = comp_color.b + color.a * color.b * comp_transp;
            comp_transp = (1.0 - color.a) * comp_transp;
        }
        else
        {
            i++;
            comp_color.r = color.r * color.a;
            comp_color.g = color.g * color.a;
            comp_color.b = color.b * color.a;
            comp_transp = 1.0 - color.a;
        }

        // increment the ray sampling position
        sampling_pos += ray_increment;

        if(comp_transp < 0.0)
            break;

        // update the loop termination condition
        inside_volume = inside_volume_bounds(sampling_pos);
    }

#if ENABLE_OPACITY_CORRECTION == 1 // Opacity Correction
    comp_transp = pow(comp_transp, sampling_distance_ref / sampling_distance);
#endif

    dst = vec4(comp_color, 1.0 - comp_transp);
#endif

    if(dst.a < 0.1){
        discard;
    }

    // return the calculated color value
    FragColor = dst;
}
