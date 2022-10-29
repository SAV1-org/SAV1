import pygame

target = "img2.png"

image = pygame.image.load(target)
width, height = image.get_size()


def rgb_to_yuv(rgb_pixel):
    #https://wikimedia.org/api/rest_v1/media/math/render/svg/c6a6284680659a7529dc4a74dc386bca3bd1f563

    r, g, b = rgb_pixel
    yp = 0.299 * r + 0.587 * g + 0.114 * b
    u = -0.147 * r + 0.289 * g + 0.436 * b
    v = 0.615 * r + 0.515 * g - 0.100 * b

    # just makes it more pleasant to look at
    yp = round(yp)
    u = round(u)
    v = round(v) 

    return (yp, u, v)

for y in range(height):
    for x in range(width):
        rgb_pixel = image.get_at((x, y))[:3]
        
        print("yuv=", rgb_to_yuv(rgb_pixel))
        #print("rgb=", rgb_pixel)
        


