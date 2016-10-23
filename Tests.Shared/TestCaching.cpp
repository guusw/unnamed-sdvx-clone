#include "stdafx.h"
#include <Shared/Invalidation.hpp>
#include <Shared/Transform2D.hpp>
#include <Tests/Tests.hpp>

Test("Invalidation.Cached")
{
	Cached<Transform2D> cachedTransform;
	TestEnsure(!cachedTransform.IsValid());

	cachedTransform = Transform2D();
	TestEnsure(cachedTransform.IsValid());

	cachedTransform.Invalidate();
	TestEnsure(!cachedTransform.IsValid());

	Transform2D newTransform = Transform2D::Translation(Vector2(2.0f)) * Transform2D::Rotation(40.0f);
	cachedTransform.Update(newTransform);
	TestEnsure(cachedTransform.IsValid());

	TestEnsure(newTransform == cachedTransform);
}

Test("Invalidation.NotifyDirty")
{
	Cached<Vector2> scale;
	NotifyDirty<float> size;

	auto UpdateScale = [&]() { scale = Vector2(size); };
	size.Notify = [&]() { scale.Invalidate(); };

	TestEnsure(!scale.IsValid());
	scale = 1.0f;
	UpdateScale();
	TestEnsure(scale.IsValid());
	TestEnsure(scale->x == size);
	TestEnsure(scale->y == size);

	size = 10.0f;
	TestEnsure(size == 10.0f);

	TestEnsure(!scale.IsValid());
	TestEnsure(scale->x < size);
	TestEnsure(scale->y < size);

	UpdateScale();
	TestEnsure(scale.IsValid());
	TestEnsure(scale->x == size);
	TestEnsure(scale->y == size);

	auto scaleCopy = scale;
	TestEnsure(scaleCopy == scale);
	TestEnsure((Vector2)scaleCopy == (Vector2)scale);
	TestEnsure(!scaleCopy.IsValid());
}