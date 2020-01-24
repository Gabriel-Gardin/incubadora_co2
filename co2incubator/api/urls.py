from django.urls import path, include
from api.views import ChartData

urlpatterns = [
    path('api/estufa1/data/', ChartData.as_view())
]
