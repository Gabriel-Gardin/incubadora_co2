from django.urls import path, include
from . import views

urlpatterns = [
    path('', views.home, name='home'),
    path('estufa1/', views.incubator, name='incubator'),
    path('contact/', views.contact, name='contact')

]
