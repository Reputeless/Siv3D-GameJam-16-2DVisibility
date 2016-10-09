# include <Siv3D.hpp>

class LightMap
{
private:

	RectF m_room;
	
	Array<Line> m_lines;

public:

	explicit LightMap(const RectF& room = RectF(640, 480))
		: m_room(room)
	{
		addObject(m_room);
	}

	void addObject(const Triangle& triangle)
	{
		m_lines.emplace_back(triangle.p0, triangle.p1);
		m_lines.emplace_back(triangle.p1, triangle.p2);
		m_lines.emplace_back(triangle.p2, triangle.p0);
	}

	void addObject(const RectF& rect)
	{
		const RectF sRect = rect.stretched(0, 1, 1, 0);
		m_lines.push_back(sRect.top);
		m_lines.push_back(sRect.right);
		m_lines.push_back(sRect.bottom);
		m_lines.push_back(sRect.left);
	}

	void addObject(const Quad& quad)
	{
		m_lines.emplace_back(quad.p[0], quad.p[1]);
		m_lines.emplace_back(quad.p[1], quad.p[2]);
		m_lines.emplace_back(quad.p[2], quad.p[3]);
		m_lines.emplace_back(quad.p[3], quad.p[0]);
	}

	void addObject(const Circle& circle, int quality = 8)
	{
		quality = Max(quality, 6);
		const double da = TwoPi / quality;
		for (auto i : step(quality))
		{
			m_lines.push_back(Line(Circular(circle.r, da * i), Circular(circle.r, da * (i + 1))).moveBy(circle.center));
		}
	}

	void addObject(const Polygon& polygon)
	{
		const Array<Vec2> outer = polygon.outer();

		for (auto i : step(outer.size()))
		{
			m_lines.emplace_back(outer[i], outer[(i + 1) % outer.size()]);
		}
	}

	const RectF& getRoom() const
	{
		return m_room;
	}

	const Array<std::pair<Vec2, Vec2>> calculateCollidePoints(const Vec2& lightPos) const
	{
		if (!m_room.stretched(-1).contains(lightPos))
		{
			return{};
		}

		Array<double> angles;
		for (const auto& line : m_lines)
		{
			const Vec2 v = line.begin - lightPos;
			angles.push_back(Atan2(v.y, v.x));
		}
		std::sort(angles.begin(), angles.end());

		Array<std::pair<Vec2, Vec2>> collidePoints;
		const double epsilon = 1e-10;
		const double maxDistance = 2.0 * Sqrt(m_room.w * m_room.w + m_room.h + m_room.h);

		for (auto angle : angles)
		{
			const double left = angle - epsilon;
			const double right = angle + epsilon;
			const Line leftRay(lightPos, lightPos + Vec2::Right.rotated(left) * maxDistance);
			const Line rightRay(lightPos, lightPos + Vec2::Right.rotated(right) * maxDistance);

			Vec2 leftCollidePoint = leftRay.end;
			Vec2 rightCollidePoint = rightRay.end;

			for (const auto& line : m_lines)
			{
				if (const auto p = leftRay.intersectsAt(line))
				{
					if (p->distanceFromSq(lightPos) < leftCollidePoint.distanceFromSq(lightPos))
					{
						leftCollidePoint = p.value();
					}
				}

				if (const auto p = rightRay.intersectsAt(line))
				{
					if (p->distanceFromSq(lightPos) < rightCollidePoint.distanceFromSq(lightPos))
					{
						rightCollidePoint = p.value();
					}
				}
			}

			collidePoints.emplace_back(leftCollidePoint, rightCollidePoint);
		}

		return collidePoints;
	}

	Array<Triangle> calculateLightTriangles(const Vec2& lightPos) const
	{
		const auto collidePoints = calculateCollidePoints(lightPos);

		Array<Triangle> triangles(collidePoints.size());

		for (auto i : step(triangles.size()))
		{
			triangles[i].set(lightPos, collidePoints[i].second, collidePoints[(i + 1) % collidePoints.size()].first);
		}

		return triangles;
	}

	void draw(const Vec2& lightPos, const ColorF& color = ColorF(1.0)) const
	{
		for (const auto& triangle : calculateLightTriangles(lightPos))
		{
			triangle.draw(color);
		}
	}
};

void Main()
{
	const Texture texture(L"floor.png");
	Cursor::SetStyle(CursorStyle::None);
	Window::Resize(1280, 720);
	Graphics::SetBackground(Color(45));

	ConstantBuffer<Float4> cb;
	const PixelShader ps(L"Light2D.hlsl");
	if (!ps)
		return;

	LightMap lightMap(Rect(40, 40, 1200, 640));
	const Array<Triangle> triangles{ Triangle(120, 120, 300, 120, 120, 500) };
	const Array<RectF> rects{ Rect(600, 40, 40, 260), RectF(440, 300, 440, 40), RectF(1040, 300, 200, 40), Rect(480, 480, 240, 100) };
	const Array<Circle> circles{ Circle(1000, 500, 80),  Circle(460, 180, 30), Circle(240, 480, 30), Circle(300, 560, 30) };
	const Array<Polygon> polygons{ Geometry2D::CreateStar(60, 0, Vec2(940, 180)) };

	for (const auto& triangle : triangles)
		lightMap.addObject(triangle);

	for (const auto& rect : rects)
		lightMap.addObject(rect);

	for (const auto& circle : circles)
		lightMap.addObject(circle, 12);

	for (const auto& polygon : polygons)
		lightMap.addObject(polygon);

	const Color objectColor = Palette::Seagreen;

	while (System::Update())
	{
		const Vec2 mousePos = Mouse::Pos();

		*cb = Float4(mousePos, 960, 0);

		Graphics2D::BeginPS(ps);
		Graphics2D::SetConstant(ShaderStage::Pixel, 1, cb);
		Graphics2D::SetBlendState(BlendState::Additive);
		{
			lightMap.draw(mousePos + Vec2(-1, 0), ColorF(0.22, 0.24, 0.21));
			lightMap.draw(mousePos + Vec2(1, 0), ColorF(0.23, 0.23, 0.22));
			lightMap.draw(mousePos + Vec2(0, 1), ColorF(0.24, 0.22, 0.19));
			lightMap.draw(mousePos + Vec2(0, -1), ColorF(0.25, 0.21, 0.20));
		}
		Graphics2D::SetBlendState(BlendState::Default);
		Graphics2D::EndPS();

		Graphics2D::SetBlendState(BlendState::Multiplicative);
		texture.map(1280, 720).draw();
		Graphics2D::SetBlendState(BlendState::Default);

		for (const auto& triangle : triangles)
			triangle.draw(objectColor);

		for (const auto& rect : rects)
			rect.draw(objectColor);

		for (const auto& circle : circles)
			circle.draw(objectColor);

		for (const auto& polygon : polygons)
			polygon.draw(objectColor);

		lightMap.getRoom().drawFrame(4, 4, Palette::Gray);

		const auto lightTriangles = lightMap.calculateLightTriangles(mousePos);
		const Circle sensor(600, 610, 20);
		bool on = false;
		
		for (const auto& triangle : lightTriangles)
		{
			if (triangle.intersects(sensor))
			{
				on = true;
				break;
			}
		}

		sensor.draw(on ? Palette::Red : Palette::Gray);

		Circle(mousePos, 20).draw(Palette::Orange).drawFrame(1, 2);
	}
}