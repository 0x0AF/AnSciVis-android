#version 300 es
precision mediump float;
precision mediump sampler2D;
precision mediump sampler3D;

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
    dy = vec3(0.0, 0.0001, 0.0);
    dz = vec3(0.0, 0.0, 0.0001);

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
    vec3 ray_increment = normalize(ray_entry_position - camera_location) * sampling_distance;
    vec3 sampling_pos = ray_entry_position + ray_increment;
    vec4 dst = vec4(0.0, 0.0, 0.0, 0.0);
    bool inside_volume = inside_volume_bounds(sampling_pos);

    if(!inside_volume)
        discard;

#if TASK == 10
    vec4 max_val = vec4(0.0, 0.0, 0.0, 0.0);

    while(inside_volume)
    {
        float s = get_sample_data(sampling_pos);
        vec4 color = texture(transfer_texture, vec2(s, s));
        max_val.r = max(color.r, max_val.r);
        max_val.g = max(color.g, max_val.g);
        max_val.b = max(color.b, max_val.b);
        max_val.a = max(color.a, max_val.a);
        sampling_pos += ray_increment;

        inside_volume = inside_volume_bounds(sampling_pos);
    }

    dst = max_val;
#endif

#if TASK == 11
    vec4 average_val = vec4(0.0, 0.0, 0.0, 0.0);
    float i = 0.0;

    while(inside_volume)
    {
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

        sampling_pos += ray_increment;

        inside_volume = inside_volume_bounds(sampling_pos);
    }

    dst = average_val;

#endif

#if TASK == 12 || TASK == 13
    while(inside_volume)
    {
        float s = get_sample_data(sampling_pos);

        if(s > iso_value)
        {
#if TASK == 13

            vec3 hit_pos = sampling_pos;
            sampling_pos = sampling_pos - ray_increment;

            float epsilon = 0.000001;

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

#if ENABLE_LIGHTNING == 1

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

#if ENABLE_SHADOWING == 1

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

        sampling_pos += ray_increment;

        inside_volume = inside_volume_bounds(sampling_pos);
    }
#endif

#if TASK == 31
    vec3 comp_color = vec3(0, 0, 0);
    float comp_transp = 1.0;
    float i = 0.0;

    while(inside_volume)
    {
        float s = get_sample_data(sampling_pos);
        vec4 color = texture(transfer_texture, vec2(s, s));

#if ENABLE_LIGHTNING == 1
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

        sampling_pos += ray_increment;

        if(comp_transp < 0.0)
            break;

        inside_volume = inside_volume_bounds(sampling_pos);
    }

#if ENABLE_OPACITY_CORRECTION == 1
    comp_transp = pow(comp_transp, sampling_distance_ref / sampling_distance);
#endif

    dst = vec4(comp_color, 1.0 - comp_transp);
#endif

    if(dst.a < 0.01)
    {
        discard;
    }

    FragColor = dst;
}
