#include "objects/textblitter.h"
#include "include_gl.h"
#include "collision.h"
#include "shader.h"
#include <iostream>
#include "font.h"

// TextBlitter

TextBlitter::TextBlitter(int x, int y, int type_id)
: FrameObject(x, y, type_id), flash_interval(0.0f), x_spacing(0), y_spacing(0),
  x_scroll(0), y_scroll(0), anim_type(BLITTER_ANIMATION_NONE),
  charmap_ref(true), callback_line_count(0), draw_image(NULL)
{
    collision = new InstanceBox(this);
}

void TextBlitter::load(const std::string & filename)
{
    Image * new_image = get_image_cache(filename, 0, 0, 0, 0,
                                        transparent_color);
    if (new_image == NULL) {
        std::cout << "Could not load Text Blitter image " << filename
            << std::endl;
        return;
    }

    image = new_image;
    image->upload_texture();
}

TextBlitter::~TextBlitter()
{
    if (!charmap_ref) {
        delete[] charmap;
        delete charmap_str;
    }
    delete collision;
}

void TextBlitter::initialize(const std::string & map_string)
{
    for (int i = 0; i < 256; i++) {
        charmap[i] = -1;
    }

    for (unsigned int i = 0; i < map_string.size(); i++) {
        unsigned char c = (unsigned char)map_string[i];
        charmap[c] = i;
    }

    image->upload_texture();
}

int TextBlitter::get_x_align()
{
    if (alignment & ALIGN_LEFT)
        return 0;
    if (alignment & ALIGN_HCENTER)
        return 1;
    if (alignment & ALIGN_RIGHT)
        return 2;
    return 0;
}

int TextBlitter::get_y_align()
{
    if (alignment & ALIGN_TOP)
        return 0;
    if (alignment & ALIGN_VCENTER)
        return 1;
    if (alignment & ALIGN_BOTTOM)
        return 2;
    return 0;
}

void TextBlitter::set_x_align(int value)
{
    alignment &= ~(ALIGN_LEFT | ALIGN_HCENTER | ALIGN_RIGHT);
    switch (value) {
        case 0:
            alignment |= ALIGN_LEFT;
            break;
        case 1:
            alignment |= ALIGN_HCENTER;
            break;
        case 2:
            alignment |= ALIGN_RIGHT;
            break;
    }
}

void TextBlitter::set_y_align(int value)
{
    alignment &= ~(ALIGN_TOP | ALIGN_VCENTER | ALIGN_BOTTOM);
    switch (value) {
        case 0:
            alignment |= ALIGN_TOP;
            break;
        case 1:
            alignment |= ALIGN_VCENTER;
            break;
        case 2:
            alignment |= ALIGN_BOTTOM;
            break;
    }
}

void TextBlitter::set_x_spacing(int value)
{
    x_spacing = value;
}

void TextBlitter::set_y_spacing(int value)
{
    y_spacing = value;
}

void TextBlitter::set_x_scroll(int value)
{
    x_scroll = value;
}

void TextBlitter::set_y_scroll(int value)
{
    y_scroll = value;
}

void TextBlitter::set_width(int w)
{
    if (w == width)
        return;
    width = w;
    collision->update_aabb();
    update_lines();
}

void TextBlitter::set_height(int h)
{
    height = h;
    collision->update_aabb();
}

void TextBlitter::set_text(const std::string & value)
{
    if (value == text && !lines.empty())
        return;
    text = value;
    update_lines();
}

void TextBlitter::append_text(const std::string & value)
{
    text += value;
    update_lines();
}

void TextBlitter::update_lines()
{
    lines.clear();

    if (text.empty()) {
        lines.push_back(LineReference(NULL, 0));
        return;
    }

    int x_add = char_width + x_spacing;

    char * text_c = &text[0];

    bool force_char = false;

    for (unsigned int i = 0; i < text.size(); i++) {
        int start = i;
        int size = 0;
        int last_space = -1;
        bool removed_space = false;

        // find start + end of line
        while (true) {
            if (i >= text.size())
                break;
            unsigned char c = (unsigned char)text_c[i];
            if (c == '\n')
                break;
            if (c == '\r') {
                i++;
                continue;
            }
            if (wrap && ((size + 1) * x_add) > width &&
                    !force_char) {
                if (size == 0)
                    force_char = true;
                if (c == ' ') {
                    size = i - start;
                    break;
                }
                if (last_space != -1) {
                    size = last_space - start;
                    i = last_space;
                }
                i--;
                break;
            }
            force_char = false;
            i++;
            if (c == ' ')
                last_space = i;
            if (c == '\r')
                continue;
            // remove one leading space
            if (wrap && i-1 == start && c == ' ' && lines.size() > 0 &&
                    !removed_space) {
                start++;
                removed_space = true;
                continue;
            }
            size++;
        }

        lines.push_back(LineReference(&text_c[start], size));
    }
}


int TextBlitter::get_line_count()
{
    return int(lines.size());
}

std::string TextBlitter::get_line(int index)
{
    if (index < 0 || index >= int(lines.size()))
        return empty_string;
    return std::string(lines[index].start, lines[index].size);
}

std::string TextBlitter::get_map_char(int i)
{
    if (charmap_str->empty())
        return empty_string;
    i = clamp(i, 0, int(charmap_str->size())-1);
    return charmap_str->substr(i, 1);
}

void TextBlitter::replace_color(int from, int to)
{
    Color color1(from);
    Color color2(to);
    replacer.replace(from, to);
}

void TextBlitter::set_transparent_color(int v)
{
    transparent_color = v;
}

void TextBlitter::update()
{
    Image * image;
    if (draw_image == NULL)
        image = this->image;
    else
        image = draw_image;

    if (!replacer.empty()) {
        draw_image = replacer.apply(image, this->image);
        draw_image->upload_texture();
    }

    update_flash(flash_interval, flash_time);

    if (anim_type != BLITTER_ANIMATION_SINWAVE)
        return;
    anim_frame++;
}

void TextBlitter::flash(float value)
{
    flash_interval = value;
    flash_time = 0.0f;
}

void TextBlitter::set_animation_type(int value)
{
    anim_type = value;
}

void TextBlitter::set_animation_parameter(int index, int value)
{
    switch (index) {
        case 1:
            wave_freq = value;
            break;
        case 2:
            wave_height = value;
            break;
        default:
            std::cout << "Invalid Text Blitter parameter: " << index
                << std::endl;
            break;
    }
}

const std::string & TextBlitter::get_charmap()
{
    return *charmap_str;
}

void TextBlitter::set_charmap(const std::string & charmap)
{
    if (charmap_ref) {
        this->charmap = new int[256];
        charmap_ref = false;
    } else {
        delete charmap_str;
    }
    charmap_str = new std::string(charmap);
    initialize(charmap);
}

void TextBlitter::draw()
{
    callback_line_count = int(lines.size());

    Image * image;
    if (draw_image == NULL)
        image = this->image;
    else
        image = draw_image;

    if (!replacer.empty()) {
        draw_image = image = replacer.apply(image, this->image);
        draw_image->upload_texture();
    }

    begin_draw();

    blend_color.apply();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, image->tex);

    int x_add = char_width + x_spacing;
    int y_add = char_height + y_spacing;

    int yy = y + y_scroll;
    if (alignment & ALIGN_VCENTER)
        yy += height / 2 - lines.size() * char_height / 2
              - (lines.size() - 1) * y_spacing;

    vector<LineReference>::const_iterator it;

    // glEnable(GL_SCISSOR_TEST);
    // glc_scissor_world(x, y, width, height);

    for (it = lines.begin(); it != lines.end(); ++it) {
        const LineReference & line = *it;

        int xx = x + x_scroll;

        if (alignment & ALIGN_HCENTER) {
            xx += (width - line.size * x_add) / 2;
        } else if (alignment & ALIGN_RIGHT) {
            xx += width - line.size * x_add;
        }

        // draw line
        for (int i = 0; i < line.size; i++) {
            unsigned char c = (unsigned char)line.start[i];
            c -= char_offset;
            int ci = charmap[c];
            int img_x = (ci * char_width) % image_width;
            img_x = clamp(img_x + x_off, 0, image->width);
            int img_y = ((ci * char_width) / image_width) * char_height;
            img_y = clamp(img_y + y_off, 0, image->height);

            float t_x1 = float(img_x) / float(image->width);
            float t_x2 = float(img_x+char_width) / float(image->width);
            float t_y1 = float(img_y) / float(image->height);
            float t_y2 = float(img_y+char_height) / float(image->height);

            int yyy = yy;
            if (anim_type == BLITTER_ANIMATION_SINWAVE) {
                double t = double(anim_frame / anim_speed + x_add * i);
                t /= double(wave_freq);
                yyy += int(sin(t) * wave_height);
            }

            glBegin(GL_QUADS);
            glTexCoord2f(t_x1, t_y1);
            glVertex2i(xx, yyy);
            glTexCoord2f(t_x2, t_y1);
            glVertex2i(xx + char_width, yyy);
            glTexCoord2f(t_x2, t_y2);
            glVertex2i(xx + char_width, yyy + char_height);
            glTexCoord2f(t_x1, t_y2);
            glVertex2i(xx, yyy + char_height);
            glEnd();

            xx += x_add;
        }

        yy += y_add;
    }

    glDisable(GL_TEXTURE_2D);
    // glDisable(GL_SCISSOR_TEST);

    end_draw();
}

class DefaultBlitter : public TextBlitter
{
public:
    DefaultBlitter()
    : TextBlitter(0, 0, 0)
    {
        setup_default_instance(this);
        collision = new InstanceBox(this);
        create_alterables();
        this->image = &dummy_image;
    }
};

static DefaultBlitter default_blitter;
FrameObject * default_blitter_instance = &default_blitter;
